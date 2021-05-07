'''Wakaama integration tests (pytest) fixtures'''
import pytest
import pexpect


# pylint: disable=no-member
class HelperBase:
    '''Provides helper methods for integration tests. Wraps "pexpect" API'''

    def __dumptext(self):
        '''Internal method that prints debug help'''
        print("Debug help: actual output---------------------")
        print(self.pexpectobj.before)
        print("----------------------------------------------")

    def commandresponse(self, cmd,resp):
        '''Issue CLI command and check response'''
        self.pexpectobj.sendline(cmd)
        try:
            self.pexpectobj.expect_exact(resp)
        except pexpect.exceptions.TIMEOUT:
            self.__dumptext()
            return False
        return True

    def waitforpacket(self):
        '''Wait for "packet printout" and return text output'''
        try:
            self.pexpectobj.expect_exact("bytes received from")
        except pexpect.exceptions.TIMEOUT:
            self.__dumptext()
            assert False
        try:
            self.pexpectobj.expect_exact("\r\r\n>")
        except pexpect.exceptions.TIMEOUT:
            self.__dumptext()
            assert False
        return self.pexpectobj.before

    def waitfortext(self,txt):
        '''Wait for text printout and return True if found'''
        try:
            self.pexpectobj.expect_exact(txt)
        except pexpect.exceptions.TIMEOUT:
            self.__dumptext()
            return False
        return True

    def quit(self):
        '''Quit client or server'''
        self.pexpectobj.sendline("q")       # exit, if client
        self.pexpectobj.sendline("quit")    # exit, if server
        self.pexpectobj.expect(pexpect.EOF) # make sure exited


class Lwm2mServer(HelperBase):
    '''Server subclass of HelperBase'''
    def __init__(self, arguments="", timeout=3, encoding="utf8"):
        self.pexpectobj = pexpect.spawn("build-wakaama/examples/server/lwm2mserver" + " " + arguments,
                               encoding=encoding,
                               timeout=timeout)
        # pylint: disable=consider-using-with
        self.pexpectobj.logfile = open("lwm2mserver_log.txt", "w")

class Lwm2mClient(HelperBase):
    '''Client subclass of HelperBase'''
    def __init__(self, arguments="", timeout=3, encoding="utf8"):
        self.pexpectobj = pexpect.spawn("build-wakaama/examples/client/lwm2mclient" + " " + arguments,
                               encoding=encoding,
                               timeout=timeout)
        # pylint: disable=consider-using-with
        self.pexpectobj.logfile = open("lwm2mclient_log.txt", "w")


class Lwm2mBootstrapServer(HelperBase):
    '''Bootstrap-server subclass of HelperBase'''
    def __init__(self, arguments="", timeout=3, encoding="utf8"):
        self.pexpectobj = pexpect.spawn("build-wakaama/examples/bootstrap_server/bootstrap_server" + " " + arguments,
                               encoding=encoding,
                               timeout=timeout)
        # pylint: disable=consider-using-with
        self.pexpectobj.logfile = open("lwm2mbootstrapserver_log.txt", "w")


@pytest.fixture
def lwm2mserver(request):
    """Provide lwm2mserver instance."""
    marker = request.node.get_closest_marker("server_args")
    if marker is None:
        args = ""
    else:
        args = marker.args[0]
    server = Lwm2mServer(args)
    yield server
    server.quit()


@pytest.fixture
def lwm2mclient(request):
    """Provide lwm2mclient instance."""
    marker = request.node.get_closest_marker("client_args")
    if marker is None:
        args = ""
    else:
        args = marker.args[0]
    client = Lwm2mClient(args)
    yield client
    client.quit()


@pytest.fixture
def lwm2mbootstrapserver():
    """Provide lwm2mclient instance."""
    bootstrapserver = Lwm2mBootstrapServer("-f examples/bootstrap_server/bootstrap_server.ini", timeout=13)
    bootstrapserver.waitfortext(">")
    yield bootstrapserver
    bootstrapserver.quit()
