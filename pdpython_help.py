import pd

class HelpClass:
    def __init__(self, *creation):
        pd.post("Creating HelpClass object with creation args: %s." % str(creation))
        self.dict = dict()
        return

    # simply
    def bang(self):
        pd.post("HelpClass object received bang.")
        return True

    def float(self, number):
        pd.post("HelpClass object received %f." % number)
        return number

    def symbol(self, string):
        pd.post("HelpClass object received symbol: '%s'." % string)
        return string

    def list(self, *args):
        pd.post("HelpClass object received list: %s." % str(args))
        return list(args)

    def goto(self, location):
        pd.post("HelpClass object received goto:" + location)
        return location

    def moveto(self, x, y, z):
        pd.post("HelpClass object received moveto: %s, %s, %s." %
                 (str(x), str(y), str(z)))
        return [x, y, z]

    def blah(self):
        pd.post("HelpClass object received blah message.")
        return 42.0

    def tuple(self):
        pd.post("Python HelpClass object received tuple message.")
        return (['element', 1], ['element', 2], ['element', 3],[ 'element',4 ])

    # dict
    def dict_set(self, key, value):
        self.dict[key] = value
        return

    def dict_get(self, key):
        try:
            return self.dict[key]
        except:
            return

    def dict_keys(self):
        return self.dict.keys()

    # array
    def array_get(self, array_name):
        l = pd.pd_array_to_list(str(array_name))
        return(l)

    def array_set(self, *args):
        l = list(args)
        if len(l) > 0:
            name = l[0]
        else:
            pd.post("error array_set: need array_name val0 val1 ... ")
            return
        l = l[1:]
        pd.list_to_pd_array(str(name), l)
        return len(l)

    def array_size(self, name):
        size = pd.pd_array_size(str(name))
        return size

    def array_resize(self, name, size):
        size = pd.pd_array_resize(str(name), int(size))
        return size

    # value
    def value_get(self, name):
        f = pd.pd_value_get(str(name))
        return f

    def value_set(self, name, f):
        pd.pd_value_set(str(name), float(f))
        return

    # send
    def send_bang(self, name):
        pd.pd_send_bang(str(name))
        return

    def send_float(self, name, f):
        pd.pd_send_float(str(name), float(f))
        return

    def send_symbol(self, name, s):
        pd.pd_send_symbol(str(name), str(s))
        return

    def send_list(self, *args):
        l = list(args)
        if len(l) > 0:
            name = l[0]
            l = l[1:]
        else:
            pd.post("error send_list: sendname val0 val1 ... ")
            return
        pd.pd_send_list(str(name), l)
        return

    # debug
    def debug_level(self, v):
        pd.debug_level(int(v))
        return

    # test
    def test(self):
        a = 10
        b = 20
        c = test_add(a, b)
        return c

    def test2(self):
        a = 10
        b = 20
        c = test_add(a, b)
        e = test_mul(a, b)
        a = test_add(c, e)
        return a

def test_add(a, b):
    return a+b

def test_mul(a, b):
    return a*b
