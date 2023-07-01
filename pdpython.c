#define PY_SSIZE_T_CLEAN

#include <Python.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "m_pd.h"

// -------------------------------------------------------------------------- //
// debug
enum
{
  DEBUG_ERROR=0,
  DEBUG_WARNING,
  DEBUG_VERBOSE,
};

int debug_level = DEBUG_ERROR;

/* #define DEBUG_S(L, S)				\ */
/*   if (debug_level >= L)				\ */
/*     {						\ */
/*       post("pdpython debug:");			\ */
/*       post(S);					\ */
/*     } */

/* #define DEBUG_SS(L, S1, S2)			\ */
/*   if (debug_level >= L)				\ */
/*     {						\ */
/*       post("pdpython debug:");			\ */
/*       post(S1, S2);				\ */
/*     } */

/* #define DEBUG_SSS(L, S1, S2, S3)			\ */
/*   if (debug_level >= L)					\ */
/*     {							\ */
/*       post("pdpython debug:");				\ */
/*       post(S1, S2, S3);					\ */
/*     } */


#define DEBUG_S(L, S)				\
  post("pdpython debug:");			\
  post(S);					

#define DEBUG_SS(L, S1, S2)			\
  post("pdpython debug:");			\
  post(S1, S2);				

#define DEBUG_SSS(L, S1, S2, S3)		\
  post("pdpython debug:");			\
  post(S1, S2, S3);					

// -------------------------------------------------------------------------- //
// struct
typedef struct pdpython
{
  t_object x_ob;
  t_outlet *x_outlet;
  PyObject *py_object;
} t_pdpython;
static t_class *pdpython_class;

// -------------------------------------------------------------------------- //
// Utility functions for object conversion.
// Attempt to convert a Pd object to a Python object.  The caller must take
// responsibility for releasing the Python object reference returned.
static PyObject *t_atom_to_PyObject(t_atom *atom) 
{
  switch(atom->a_type)
    {
    case A_FLOAT:
      return PyFloat_FromDouble(atom_getfloat(atom));
    case A_SYMBOL:
      return PyUnicode_FromString(atom->a_w.w_symbol->s_name);
    case A_NULL:
      Py_RETURN_NONE;
    default:
      // A_POINTER
      // A_SEMI
      // A_COMMA
      // A_DEFFLOAT
      // A_DEFSYM
      // A_DOLLAR
      // A_DOLLSYM
      // A_GIMME
      // A_CANT
      // A_BLOB
      DEBUG_SS(DEBUG_WARNING,
	       "Type %d unsupported for conversion to Python.",
	       atom->a_type);
      Py_RETURN_NONE;
    }
}

// -------------------------------------------------------------------------- //
// Convert a list of Pd atoms into a Python list.
static PyObject *t_atom_list_to_PyObject_list(int argc, t_atom *argv)
{
  PyObject *list = PyTuple_New(argc);
  int i;
  for (i=0; i<argc; i++)
    {
      PyObject *value = t_atom_to_PyObject(&argv[i]);
      PyTuple_SetItem(list, i, value);
    }
  return list;
}

// -------------------------------------------------------------------------- //
// Set a Pd atom structure to a representation of an object of atomic concrete Python type.
static void PyObject_to_atom(PyObject *value, t_atom *atom)
{
  if      (value == Py_True)      SETFLOAT(atom, (t_float) 1.0);
  else if (value == Py_False)     SETFLOAT(atom, (t_float) 0.0);
  else if (PyFloat_Check(value))  SETFLOAT(atom, (t_float) PyFloat_AsDouble(value));
  else if (PyLong_Check(value))   SETFLOAT(atom, (t_float) PyLong_AsLong(value));
  /* else if (PyInt_Check(value))    SETFLOAT(atom, (t_float) PyLong_AsLong(value)); */
  else if (PyUnicode_Check(value))
    {
      /* SETSYMBOL(atom, gensym(PyUnicode_AsString(value))); */
      SETSYMBOL(atom, gensym((char *)PyUnicode_AsWideCharString(value, NULL)));
    }
  else
    SETSYMBOL(atom, gensym("error"));
}

// -------------------------------------------------------------------------- //
// Create a newly allocated list of Pd atoms from a Python list.  The caller is
// responsible for freeing the memory block allocated with malloc() and
// returned in *argv.  Pd lists cannot be nested, e.g,. they are only 1D
// arrays, so an error token is substituted for any non-atomic object.
static void new_list_from_sequence(PyObject *seq, int *argc, t_atom **argv)
{
  Py_ssize_t len = 0;
  Py_ssize_t i;
  if (PyList_Check(seq))
    {
      len = PyList_Size(seq);
      *argv = (t_atom *) malloc(len*sizeof(t_atom));
      for (i=0; i<len; i++)
	{
	  PyObject *elem = PyList_GetItem(seq, i);
	  PyObject_to_atom(elem, (*argv) + i);
	}
    }
  else
    {
      DEBUG_S(DEBUG_WARNING, "bad list.");
    }
  *argc = (int) len;
}

// -------------------------------------------------------------------------- //
// Emit a Python object as an outlet message.  Tuples generate multiple
// messages and are handled separately.
static void emit_outlet_message(PyObject *v, t_outlet *x_outlet)
{
  post("emit");
  if      (v == Py_True)      outlet_float(x_outlet, 1.0);
  else if (v == Py_False)     outlet_float(x_outlet, 0.0);
  else if (PyFloat_Check(v))  outlet_float(x_outlet, (float) PyFloat_AsDouble(v));
  else if (PyLong_Check(v))   outlet_float(x_outlet, (float) PyLong_AsLong(v));
  /* else if (PyInt_Check(v))    outlet_float(x_outlet, (float) PyLong_AsLong(v)); */
  else if (PyUnicode_Check(v))
    {
      outlet_symbol(x_outlet, gensym((char *)PyUnicode_AsWideCharString(v, NULL)));
    }
  else if (PyList_Check(v))
    {
      // Create an atom array representing a 1D Python list.
      t_atom *argv = NULL;
      int argc = 0;
      new_list_from_sequence(v, &argc, &argv);
      if (argc > 0)
	{
	  // Follow the Pd rules for interpreting lists.
	  // If the first element is a symbol, then treat it as
	  // the 'selector', otherwise treat all elements as data.
	  if (argv[0].a_type == A_SYMBOL)
	    {
	      outlet_anything(x_outlet, atom_getsymbol(&argv[0]), argc-1, argv+1);      
	    }
	  else
	    {
	      outlet_list(x_outlet, &s_list, argc, argv);
	    }
	}
      if (argv) free (argv);
    }
  // else nothing
}

// -------------------------------------------------------------------------- //
// Call a method of the associated Python object based on the inlet value.
// Message types are handled as follows:
// bang       : obj.bang()            example: [bang]   calls obj.bang() 
// float      : obj.float(number)     example: [1.0]    calls obj.float(1.0)
// list (num) : obj.list(a1, a2, ...) example: [1 2 3]  calls obj.list(1.0, 2.0, 3.0)
// list (sel) : obj.$selector(string) example: [goto 4] calls obj.goto(4.0)
// more examples:
// [ blah ]      is a list with a selector and null values, calls obj.blah()
// [ goto home ] is a list with a selector and a symbol, calls obj.blah("home")
// a weird special case:
// symbol  : obj.symbol(string)  example: [ symbol ] calls obj.symbol("symbol")
static void pdpython_eval(t_pdpython *x, t_symbol *selector, int argcount, t_atom *argvec)
{
  /* DEBUG_SSS(DEBUG_VERBOSE, */
	    post("pdpython_eval called with %d args, selector %s",
	    argcount, selector->s_name);
  PyObject *func = NULL;
  PyObject *args = NULL;
  PyObject *value = NULL;
  if (x->py_object == NULL)
    {
      /* DEBUG_S(DEBUG_WARNING, */
      post( "message sent to uninitialized python object.");
      return;
    }
  func = PyObject_GetAttrString(x->py_object, selector->s_name);
  args = t_atom_list_to_PyObject_list(argcount, argvec);
  if (!func) 
    {
      /* DEBUG_SS(DEBUG_WARNING, */
      post( "no Python function found for selector %s.", 
	       selector->s_name);
    }
  else 
    {
      if (!PyCallable_Check(func)) 
	{
	  /* DEBUG_SS(DEBUG_WARNING, */
	  post( "Python attribute for selector %s is not callable.",
		   selector->s_name);
	}
      else 
	{
	  value = PyObject_CallObject(func, args);
	}
      Py_DECREF(func);
    }

  if (args) Py_DECREF(args);
  if (value == NULL) 
    {
      /* DEBUG_SS(DEBUG_WARNING, */
      post( "Python call for selector %s failed.",
		   selector->s_name);
    }
  else 
    {
      if (PyTuple_Check(value)) 
	{
	  // A tuple generates a sequence of outlet messages, one per item.
	  int i, len = (int) PyTuple_Size(value);
	  for (i=0; i<len; i++) 
	    {
	      PyObject *elem = PyTuple_GetItem(value, i);
	      emit_outlet_message(elem, x->x_outlet);
	    }
	} 
      else
	{
	  emit_outlet_message(value, x->x_outlet);
	}
      Py_DECREF(value);
    }
}

// -------------------------------------------------------------------------- //
// Define an internal 'pdgui' module with a post() method to allow Python
// programs to print messages on the Pd console.  This function is the C
// wrapper function called by Python for the Pd post() function.  This is the
// only means for Python to make calls back into Pd.
// The argument must be a single string.  Formatting can be handled in Python.
static PyObject* pd_post(PyObject *self __attribute__((unused)), PyObject *args)
{
  char *text;
  if(!PyArg_ParseTuple(args, "s", &text))
    {
      DEBUG_S(DEBUG_WARNING,
	      "unprintable object posted to the console from a python object.");
    }
  else
    {
      post(text);
    }
  Py_RETURN_NONE;
}

// -------------------------------------------------------------------------- //
// set debug level messages
static PyObject* pd_debug(PyObject *self __attribute__((unused)), PyObject *args)
{
  int v;
  if(!PyArg_Parse(args, "(i)", &v))
    {
      DEBUG_S(DEBUG_ERROR, "bad arguments: int");
    }
  else
    {
      debug_level=v;
    }
  Py_RETURN_NONE;
}

// -------------------------------------------------------------------------- //
// open pd array
static int pd_open_array(t_symbol *s_arr,  // name
			 t_word **w_arr,   // word
			 t_garray **g_arr) // garray
{
  int len;
  t_word *i_w_arr;
  t_garray *i_g_arr;
  if (!(i_g_arr = (t_garray *)pd_findbyclass(s_arr, garray_class)))
    {
      DEBUG_SS(DEBUG_ERROR, "no such array: %s", s_arr->s_name);
      len = -1;
    }
  else if (!garray_getfloatwords(i_g_arr, &len, &i_w_arr))
    {
      DEBUG_SS(DEBUG_ERROR, "bad template: %s", s_arr->s_name);
      len = -1;
    }
  else
    {
      *w_arr = i_w_arr;
      *g_arr = i_g_arr;
    }
  return (len);
}

// -------------------------------------------------------------------------- //
// pdarray -> pylist
static PyObject* pd_array_to_list(PyObject *self __attribute__((unused)), PyObject *args)
{
  char *array_name;
  if(!PyArg_Parse(args, "(s)", &array_name))
    {
      DEBUG_S(DEBUG_ERROR, "bad arguments: string");
      Py_RETURN_NONE;
    }
  else
    {
      t_word   *w;
      t_garray *g;
      int       a_len;
      t_symbol *s = gensym(array_name);
      a_len = pd_open_array(s, &w, &g);
      if (a_len <= 0)
	{
	  Py_RETURN_NONE;
	}
      else
	{
	  PyObject *list = PyList_New(a_len);
	  int i;
	  for (i=0; i<a_len; i++)
	    {
	      PyObject *value = PyFloat_FromDouble(w[i].w_float);
	      PyList_SetItem(list, i, value);
	    }
	  return list;
	}
    }
}

// -------------------------------------------------------------------------- //
// pylist -> pdarray
static PyObject* list_to_pd_array(PyObject *self __attribute__((unused)), PyObject *args)
{
  char *array_name;
  PyObject  *in_list;
  if(!PyArg_Parse(args, "(sO)", &array_name, &in_list))
    {
      DEBUG_S(DEBUG_ERROR, "bad arguments: string list");
      Py_RETURN_NONE;
    }
  else
    {
      if (PyList_Check(in_list) == 1)
	{
	  t_word   *w;
	  t_garray *g;
	  int       a_len;
	  t_symbol *s = gensym(array_name);
	  a_len = pd_open_array(s, &w, &g);
	  if (a_len <= 0)
	    {
	      Py_RETURN_NONE;
	    }
	  else
	    {
	      // resize
	      Py_ssize_t l_len = PyList_Size(in_list);
	      if (l_len != a_len)
		{
		  garray_resize(g, l_len);
		}
	      a_len = pd_open_array(s, &w, &g);
	      int i;
	      for (i=0; i<a_len; i++)
		{
		  PyObject *v = PyList_GetItem(in_list, i);
		  if     (v == Py_True)     w[i].w_float=1.0;
		  else if(v == Py_False)    w[i].w_float=0.0;
		  else if(PyFloat_Check(v)) w[i].w_float=(float)PyFloat_AsDouble(v);
		  else if(PyLong_Check(v))  w[i].w_float=(float)PyLong_AsLong(v);
		  /* else if(PyInt_Check(v))   w[i].w_float=(float)PyLong_AsLong(v); */
		  else                      w[i].w_float=0.0;
		}
	      garray_redraw(g);
	      Py_RETURN_NONE;
	    }
	}
      else
	{
	  DEBUG_S(DEBUG_ERROR, "bad arguments: list string");
	  Py_RETURN_NONE;
	}
    }
}

// -------------------------------------------------------------------------- //
// size pdarray
static PyObject* pd_array_size(PyObject *self __attribute__((unused)), PyObject *args)
{
  char *array_name;
  if(!PyArg_Parse(args, "(s)", &array_name))
    {
      DEBUG_S(DEBUG_ERROR, "bad arguments: string");
      Py_RETURN_NONE;
    }
  else
    {
      t_word   *w;
      t_garray *g;
      int       a_len;
      t_symbol *s = gensym(array_name);
      a_len = pd_open_array(s, &w, &g);
      return (Py_BuildValue("i", a_len));
    }
}

// -------------------------------------------------------------------------- //
// resize pdarray
static PyObject* pd_array_resize(PyObject *self __attribute__((unused)), PyObject *args)
{
  int len;
  char *array_name;
  if(!PyArg_Parse(args, "(si)", &array_name, &len))
    {
      DEBUG_S(DEBUG_ERROR, "bad arguments: string int");
      Py_RETURN_NONE;
    }
  else
    {
      if (len < 1) len = 1;
      t_word   *w;
      t_garray *g;
      int       a_len;
      t_symbol *s = gensym(array_name);
      a_len = pd_open_array(s, &w, &g);
      if (a_len <= 0)
	{
	  Py_RETURN_NONE;
	}
      else
	{
	  garray_resize(g, len);
	  garray_redraw(g);
	  return (Py_BuildValue("i", len));
	}
    }
}

// -------------------------------------------------------------------------- //
// get pd value
static PyObject* pd_value_get(PyObject *self __attribute__((unused)), PyObject *args)
{
  char *name;
  if(!PyArg_Parse(args, "(s)", &name))
    {
      DEBUG_S(DEBUG_ERROR, "bad arguments: string");
      Py_RETURN_NONE;
    }
  else
    {
      t_symbol *s = gensym(name);
      t_float *x_floatstar;
      x_floatstar = value_get(s);
      return (Py_BuildValue("f", *x_floatstar));
    }
}

// -------------------------------------------------------------------------- //
// set pd value
static PyObject* pd_value_set(PyObject *self __attribute__((unused)), PyObject *args)
{
  char *name;
  float f;
  PyObject  *in_f;
  if(!PyArg_Parse(args, "(sO)", &name, &in_f))
    {
      DEBUG_S(DEBUG_ERROR, "bad arguments: string float");
      Py_RETURN_NONE;
    }
  else
    {
      if (PyFloat_Check(in_f) == 1)
	{
	  f = PyFloat_AsDouble(in_f);
	}
      else
	{
	  DEBUG_S(DEBUG_ERROR, "bad arguments: string float");
	  Py_RETURN_NONE;
	}
      t_symbol *s = gensym(name);
      t_float *x_floatstar;
      x_floatstar = value_get(s);
      *x_floatstar = (t_float)f;
      Py_RETURN_NONE;
    }
}

// -------------------------------------------------------------------------- //
// send bang to pd receive
static PyObject* pd_send_bang(PyObject *self __attribute__((unused)), PyObject *args)
{
  char *name;
  if(!PyArg_Parse(args, "(s)", &name))
    {
      DEBUG_S(DEBUG_ERROR, "bad arguments: string");
      Py_RETURN_NONE;
    }
  else
    {
      t_symbol *s = gensym(name);
      if (s->s_thing)
	pd_bang(s->s_thing);
      Py_RETURN_NONE;
    }
}

// -------------------------------------------------------------------------- //
// send float to pd receive
static PyObject* pd_send_float(PyObject *self __attribute__((unused)), PyObject *args)
{
  char *name;
  float f;
  PyObject  *in_f;
  if(!PyArg_Parse(args, "(sO)", &name, &in_f))
    {
      DEBUG_S(DEBUG_ERROR, "bad arguments: string float");
      Py_RETURN_NONE;
    }
  else
    {
      if (PyFloat_Check(in_f) == 1)
	{
	  f = PyFloat_AsDouble(in_f);
	}
      else
	{
	  DEBUG_S(DEBUG_ERROR, "bad arguments: string float");
	  Py_RETURN_NONE;
	}
      t_symbol *s = gensym(name);
      if (s->s_thing)
	pd_float(s->s_thing, f);
      Py_RETURN_NONE;
    }
}

// -------------------------------------------------------------------------- //
// send symbol to pd receive
static PyObject* pd_send_symbol(PyObject *self __attribute__((unused)), PyObject *args)
{
  char *name;
  char *sym;
  if(!PyArg_Parse(args, "(ss)", &name, &sym))
    {
      DEBUG_S(DEBUG_ERROR, "bad arguments: string string");
      Py_RETURN_NONE;
    }
  else
    {
      t_symbol *s = gensym(name);
      if (s->s_thing)
	pd_symbol(s->s_thing, gensym(sym));
      Py_RETURN_NONE;
    }
}

// -------------------------------------------------------------------------- //
// send list to pd receive
static PyObject* pd_send_list(PyObject *self __attribute__((unused)), PyObject *args)
{
  char *name;
  PyObject *in_list;
  if(!PyArg_Parse(args, "(sO)", &name, &in_list))
    {
      DEBUG_S(DEBUG_ERROR, "bad arguments: string list");
      Py_RETURN_NONE;
    }
  else
    {
      t_atom *argv = NULL;
      int argc = 0;
      new_list_from_sequence(in_list, &argc, &argv);
      t_symbol *s = gensym(name);
      if (s->s_thing)
	{
	  if (argc > 0)
	    {
	      // Follow the Pd rules for interpreting lists.
	      // If the first element is a symbol, then treat it as
	      // the 'selector', otherwise treat all elements as data.
	      if (argv[0].a_type == A_SYMBOL)
		{
		  typedmess(s->s_thing, atom_getsymbol(&argv[0]), argc-1, argv+1);      
		}
	      else
		{
		  pd_list(s->s_thing, &s_list, argc, argv);
		}
	    }
	}
      if (argv) free (argv);
      Py_RETURN_NONE;
    }
}

// -------------------------------------------------------------------------- //
// Define the pd module for C callbacks from Python to the Pd system.
static PyMethodDef pd_methods[] = {
  { "post",             pd_post,           METH_VARARGS, "print pd console" },
  { "debug",            pd_debug,          METH_VARARGS, "set a debug level" },
  { "pd_array_to_list", pd_array_to_list,  METH_VARARGS, "pdarray -> list" },
  { "list_to_pd_array", list_to_pd_array,  METH_VARARGS, "list -> pdarray" },
  { "pd_array_size",    pd_array_size,     METH_VARARGS, "size pdarray" },
  { "pd_array_resize",  pd_array_resize,   METH_VARARGS, "resize pdarray" },
  { "pd_value_get",     pd_value_get,      METH_VARARGS, "get pd value" },
  { "pd_value_set",     pd_value_set,      METH_VARARGS, "set pd value" },
  { "pd_send_bang",     pd_send_bang,      METH_VARARGS, "send bang to pd receive" },
  { "pd_send_float",    pd_send_float,     METH_VARARGS, "send float to pd receive" },
  { "pd_send_symbol",   pd_send_symbol,    METH_VARARGS, "send symbol to pd receive" },
  { "pd_send_list",     pd_send_list,      METH_VARARGS, "send list to pd receive" },
  { NULL,               NULL,              0,            NULL }
};

// -------------------------------------------------------------------------- //
// struct modul
static struct PyModuleDef pd_module = 
  {
    PyModuleDef_HEAD_INIT,
    "pd",                                   // name
    "python for pd",     // desc or NULL 
    -1,                                        // size struct for everyone
    pd_methods,                             // methods table
    NULL,                                      // m_slots
    NULL,                                      // m_traverse
    NULL,                                      // m_clear
    NULL                                       // m_free
  };


// -------------------------------------------------------------------------- //
// init
PyMODINIT_FUNC PyInit_pd()
{
  return PyModule_Create(&pd_module);
}

// -------------------------------------------------------------------------- //
// Create an instance of a Pd 'python' object.
// The creation arguments are treated as follows:
//    module_name function_name [arg]*
// The Python function must return a Python callable object which can be called
// with messages.  It is typically a class allocator.
static void *pdpython_new(t_symbol *selector, int argcount, t_atom *argvec)
{
  t_pdpython *x = (t_pdpython *) pd_new(pdpython_class);
  x->py_object = NULL;
  DEBUG_SSS(DEBUG_VERBOSE,
	    "pdpython_new called with selector %s and argcount %d",
	    selector->s_name, argcount);
  if (argcount < 2) 
    {
      DEBUG_S(DEBUG_ERROR,
	      "python objects require a module and function specified in the creation arguments.");
    }
  else 
    {
      // Add the current canvas path to the Python load path if not already
      // present.  This will help the module import to find Python modules
      // located in the same folder as the patch.
      t_symbol *canvas_path = canvas_getcurrentdir();

      /* post("canvas_path: %s", canvas_path->s_name); */
      /* PyRun_SimpleString("import sys"); */
      /* char buf[1024]; */
      /* sprintf(buf, "sys.path.append(\"%s\")", canvas_path->s_name); */
      /* /\* PyRun_SimpleString("sys.path.append(\"%s\")", canvas_path->s_name); *\/ */
      /* PyRun_SimpleString(buf); */

      /* PyObject* modulePath = PyString_FromString(canvas_path->s_name); */
      PyObject* modulePath = PyUnicode_FromString(canvas_path->s_name);
      PyObject* sysPath    = PySys_GetObject((char*) "path"); // borrowed reference
      if (!PySequence_Contains(sysPath, modulePath))
	{
	  DEBUG_SS(DEBUG_VERBOSE,
		   "Appending current canvas path to Python load path: %s",
		   canvas_path->s_name);
	  PyList_Append(sysPath, modulePath);
	}
      Py_DECREF(modulePath);
      // try loading the module
      PyObject *module_name   = t_atom_to_PyObject(&argvec[0]);
      PyObject *module        = PyImport_Import(module_name);
      Py_DECREF(module_name);
      if (module == NULL) 
	{
	  DEBUG_SS(DEBUG_ERROR,
		   "unable to import Python module %s.",
		   argvec[0].a_w.w_symbol->s_name);
	}
      else
	{
	  PyObject *func = PyObject_GetAttrString(module, argvec[1].a_w.w_symbol->s_name);
	  if (func == NULL)
	    {
	      DEBUG_SS(DEBUG_ERROR,
		       "Python function %s not found.",
		       argvec[1].a_w.w_symbol->s_name);
	    }
	  else
	    {
	      if (!PyCallable_Check(func))
		{
		  DEBUG_SS(DEBUG_ERROR,
			   "Python attribute %s is not callable.",
			   argvec[1].a_w.w_symbol->s_name);
		}
	      else
		{
		  PyObject *args = t_atom_list_to_PyObject_list(argcount-2, argvec+2);
		  x->py_object   = PyObject_CallObject(func, args);
		  Py_DECREF(args);
		}
	      Py_DECREF(func);
	    }
	  Py_DECREF(module);
	}
    }
  // create an outlet on which to return values
  x->x_outlet = outlet_new(&x->x_ob, NULL);
  return (void *)x;
}

// -------------------------------------------------------------------------- //
// Release an instance of a Pd 'python' object.
static void pdpython_free(t_pdpython *x)
{
  DEBUG_S(DEBUG_VERBOSE, "python freeing object");
  if (x)
    {
      outlet_free(x->x_outlet);
      if (x->py_object) Py_DECREF(x->py_object);
      x->x_outlet = NULL;
      x->py_object = NULL;
    }
}

// -------------------------------------------------------------------------- //
// Initialization entry point for the Pd 'python' external.  This is
// automatically called by Pd after loading the dynamic module to initialize
// the class interface.
void pdpython_setup(void)
{
  pdpython_class = class_new(gensym("pdpython"),             // t_symbol *name
			      (t_newmethod) pdpython_new,    // t_newmethod newmethod
			      (t_method) pdpython_free,      // t_method freemethod
			      sizeof(t_pdpython),            // size_t size
			      0,                             // int flags
			      A_GIMME, 0);                   // t_atomtype arg1, ...
  class_addanything(pdpython_class, (t_method) pdpython_eval);


  if (PyImport_AppendInittab("pd", PyInit_pd) == -1) {
    fprintf(stderr, "Error: could not extend in-built modules table\n");
    /* exit(1); */
  }

  Py_SetProgramName((wchar_t *)"puredata");
  Py_Initialize();


  /* static char *arg0 = NULL; */
  /* PySys_SetArgv(0, &arg0); */

  PySys_SetArgv(0, NULL);
  /* PyInit_pd(); */

  /* if (Py_InitModule("pd", pd_methods) == NULL) */
  /*   { */
  /*     DEBUG_S(DEBUG_ERROR, "unable to create the pd module."); */
  /*   } */
}