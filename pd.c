#include <Python.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "m_pd.h"

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
      post("no such array: %s", s_arr->s_name);
      len = -1;
    }
  else if (!garray_getfloatwords(i_g_arr, &len, &i_w_arr))
    {
      post("bad template: %s", s_arr->s_name);
      len = -1;
    }
  else
    {
      *w_arr = i_w_arr;
      *g_arr = i_g_arr;
    }
  return (len);
}

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

// Define the pd module for C callbacks from Python to the Pd system.
static PyMethodDef pd_methods[] = {
  { "post",             pd_post,           METH_VARARGS, "print pd console" },
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

static struct PyModuleDef pd_module = 
  {
    PyModuleDef_HEAD_INIT,
    "pd",                                      // name
    "python for pd",                           // desc or NULL 
    -1,                                        // size struct for everyone
    pd_methods,                                // methods table
    NULL,                                      // m_slots
    NULL,                                      // m_traverse
    NULL,                                      // m_clear
    NULL                                       // m_free
  };

PyMODINIT_FUNC PyInit_pd()
{
  return PyModule_Create(&pd_module);
}
