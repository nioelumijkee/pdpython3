# Python class to support the help patch for the [python] object.

# Import the callback module built in to the external.
import pd

# Define a class which can generate objects to be instantiated in Pd [python] objects.
# This would be created in Pd as [python demo_class DemoClass].
class HelpClass:

    # The class initializer receives the creation arguments specified in the Pd [python] object.
    def __init__( self, *creation ):
        pd.post( "Creating HelpClass object with creation args: %s." % str(creation))
        # create an anonymous dictionary
        self.dict = dict()
        return

    # The following methods are called in response to Pd basic data messages.
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
        # note that the *args form provides a tuple
        pd.post( "HelpClass object received list: %s." % str(args))
        return list(args)


    # Messages with selectors are analogous to function calls, and call the
    # corresponding method.  E.g., the following could called by sending the Pd
    # message [goto 33(.
    def goto(self, location ):
        pd.post( "HelpClass object received goto:" + location)
        return location

    # This method demonstrates returning a list, which returns as a Pd list.
    def moveto( self, x, y, z ):
        pd.post( "HelpClass object received moveto: %s, %s, %s." % (str(x), str(y), str(z)))
        return [x, y, z]

    def blah(self):
        pd.post( "HelpClass object received blah message.")
        return 42.0

    # This method demostrates returning a tuple, each element of which generates
    # a separate Pd outlet message.
    def tuple(self):
        pd.post( "Python HelpClass object received tuple message.")
        return ( ['element', 1], ['element', 2], ['element', 3],[ 'element',4 ] )

    # dicts
    def dict_set( self, key, value ):
        """Set a key-value pair."""
        self.dict[key] = value

    def dict_get( self, key ):
        """Fetch the value for a given key."""
        try:
            return self.dict[key]
        except:
            return

    def dict_keys( self ):
        """Return a list of all keys."""
        return self.dict.keys()

    # pd arrays
    def array_get(self, array_name, start, length):
        """Open Pd array and make list."""
        a = pd.array_get(str(array_name), int(start), int(length))
        return(a)

    def array_set(self, array_name, x, y, z):
        """Open Pd array and set elements."""
        a = [x, y, z]
        pd.array_set(a, str(array_name), 0, 3)
        return

    def array_resize(self, array_name, size):
        """Resize Pd array."""
        pd.array_resize(str(array_name), int(size))
        return

    # disable/enable verbose messages
    def verbose(self, v):
        pd.verbose(int(v))
        return

