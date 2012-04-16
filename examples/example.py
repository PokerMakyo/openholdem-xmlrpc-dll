import xmlrpclib

from SimpleXMLRPCServer import SimpleXMLRPCServer, SimpleXMLRPCRequestHandler

port = 8080

class Bot(object):
    def __init__(self):
        self.LOADED = False
        self.state = None
        self.players = None
        self.c = xmlrpclib.ServerProxy('http://localhost:9091/RPC2')

    def getSymbol(self, chair, symbol):
        return self.c.getSymbol(chair, symbol)

    def process_message(self, message, s):
        if message == 'event':
            if not self.LOADED:
                self.LOADED = True
                print 'DLL loaded.'
            else:
                self.LOADED = False
                print 'DLL unloaded.'

        elif(message == 'state'):
            print 'We got state message!'
            self.state = s
            self.players = s['m_player']

        elif(message == 'query'):
            if s == 'dll$iswait':
                print 'Print OpenHoldem is waiting for me!'
                print 'Let\'s ask for some symbol...'
                print 'Balance:', self.getSymbol(0, 'balance')
                print 'done!'
                return 0

            if s == 'dll$rais':
                print 'Should I rais? YEEESSS!!!'
                return 1 # if want raise, return 1

            if s == 'dll$call':
                print 'Should I call?'
                return 0 # if want call, return 1

            if s == 'dll$prefold':
                print 'Should I fold?'
                return 0 # if want fold, return 1
        return 0 # hm?


class Handler(SimpleXMLRPCRequestHandler):
    def _dispatch(self, method, params):
        try:
            value = self.server.funcs[method](*params)
        except:
            import traceback
            traceback.print_exc()
            raise
        return value

if __name__ == '__main__':
    server = SimpleXMLRPCServer(("localhost", port), Handler, logRequests = False)
    print "Listening on port %s..." % port
    b = Bot()
    server.register_function(b.process_message)
    server.serve_forever()

# vim: filetype=python syntax=python expandtab shiftwidth=4 softtabstop=4
