import pytest
import pexpect


class HelperBase:
    def __dumptext(self):
        print("Debug help: actual output---------------------")
        print(self.pexpectobj.before)
        print("----------------------------------------------")

    def commandresponse(self, cmd,resp):
        self.pexpectobj.sendline(cmd)
        try:
            self.pexpectobj.expect_exact(resp)
        except pexpect.exceptions.TIMEOUT:
            self.__dumptext()
            return False
        return True

    def waitforpacket(self):
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
        try:
            self.pexpectobj.expect_exact(txt)
        except pexpect.exceptions.TIMEOUT:
            self.__dumptext()
            return False
        return True

    def quit(self):
        self.pexpectobj.sendline("q")
        self.pexpectobj.sendline("quit")
        #self.pexpectobj.expect(pexpect.EOF)


class Lwm2mServer(HelperBase):
    def __init__(self, arguments="", timeout=3, encoding="utf8"):
        self.pexpectobj = pexpect.spawn("build-wakaama/examples/server/lwm2mserver" + " " + arguments,
                               encoding=encoding,
                               timeout=timeout)
        self.pexpectobj.logfile = open("lwm2mserver_log.txt", "w")

class Lwm2mClient(HelperBase):
    def __init__(self, arguments="", timeout=3, encoding="utf8"):
        self.pexpectobj = pexpect.spawn("build-wakaama/examples/client/lwm2mclient" + " " + arguments,
                               encoding=encoding,
                               timeout=timeout)
        self.pexpectobj.logfile = open("lwm2mclient_log.txt", "w")


class Lwm2mBootstrapServer(HelperBase):
    def __init__(self, arguments="", timeout=3, encoding="utf8"):
        self.pexpectobj = pexpect.spawn("build-wakaama/examples/bootstrap_server/bootstrap_server" + " " + arguments,
                               encoding=encoding,
                               timeout=timeout)
        self.pexpectobj.logfile = open("lwm2mbootstrapserver_log.txt", "w")


@pytest.fixture
def lwm2mserver():
    """Provide lwm2mserver instance."""
    server = Lwm2mServer()
    yield server
    server.quit()


@pytest.fixture
def lwm2mclient():
    """Provide lwm2mclient instance."""
    client = Lwm2mClient()
    client.waitfortext("STATE_READY")
    yield client
    client.quit()


@pytest.fixture
def lwm2mclient_boot():
    """Provide lwm2mclient instance."""
    client = Lwm2mClient("-b -n apa")
    yield client
    client.quit()


@pytest.fixture
def lwm2mclient_text():
    """Provide lwm2mclient instance."""
    client = Lwm2mClient("-f 0")
    yield client
    client.quit()


@pytest.fixture
def lwm2mclient_json():
    """Provide lwm2mclient instance."""
    client = Lwm2mClient("-f 11543")
    yield client
    client.quit()


@pytest.fixture
def lwm2mbootstrapserver():
    """Provide lwm2mclient instance."""
    bootstrapserver = Lwm2mBootstrapServer("-f examples/bootstrap_server/bootstrap_server.ini")
    bootstrapserver.waitfortext(">")
    #LWM2M Bootstrap Server now listening on port
    yield bootstrapserver
    bootstrapserver.quit()
