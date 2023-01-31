import pd

class HelpClass:
    def __init__( self, *creation ):
        pd.post( "Creating HelpClass object with creation args: %s." % str(creation))
        self.dict = dict()
        return

    def bang(self):
        pd.post( "HelpClass object received bang.")
        return True

    def float(self, number):
        pd.post( "HelpClass object received %f." % number)
        return number

    def symbol(self, string ):
        pd.post( "HelpClass object received symbol: '%s'." % string)
        return string

    def list(self, *args ):
        pd.post( "HelpClass object received list: %s." % str(args))
        return list(args)

    def goto(self, location ):
        pd.post( "HelpClass object received goto:" + location)
        return location

    def moveto( self, x, y, z ):
        pd.post( "HelpClass object received moveto: %s, %s, %s." %
                 (str(x), str(y), str(z)))
        return [x, y, z]

    def blah(self):
        pd.post( "HelpClass object received blah message.")
        return 42.0

    def tuple(self):
        pd.post( "Python HelpClass object received tuple message.")
        return ( ['element', 1], ['element', 2], ['element', 3],[ 'element',4 ] )

    def dict_set( self, key, value):
        self.dict[key] = value
        return

    def dict_get( self, key ):
        try:
            return self.dict[key]
        except:
            return

    def dict_keys( self ):
        return self.dict.keys()

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
        pd.list_to_pd_array(l, str(name))
        return len(l)

    def array_size(self, name):
        size = pd.pd_array_size(str(name))
        return size

    def array_resize(self, name, size):
        size = pd.pd_array_resize(str(name), int(size))
        return size

    # debug
    def debug_level(self, v):
        pd.debug_level(int(v))
        return

    # tests
    def MyTest1(self):
        count = 0
        for i in range(5000):
            k = (i+1)*0.01
            for j in range(1000):
                l = (j+1)*0.01
                count += l*k
        # pd.post( "MyTest1")
        return count
