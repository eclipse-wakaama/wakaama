'''Wakaama integration tests (pytest)'''
import re


def parse_client_registration(server_output):
    """
    Output from example server with example client:

    ...
    New client #0 registered.
    Client #0:
        name: "testlwm2mclient"
        version: "1.1"
        binding: "UDP"
        lifetime: 300 sec
        objects: /1 (1.1), /1/0, /2/0, /3/0, /4/0, /5/0, /6/0, /7/0, /31024 (1.0), /31024/10, /31024/11, /31024/12,
    """
    client_id, = re.findall(r".lient #([0-9]+) ", server_output)
    event, = re.findall(r"(registered|updated)", server_output)
    endpoint, = re.findall(r"name: \"([\w-]+)\"\r\r\n", server_output)
    version, = re.findall(r"version: \"([0-9.]+)\"\r\r\n", server_output)
    binding, = re.findall(r"binding: \"([A-Z]+)\"\r\r\n", server_output)
    lifetime, = re.findall(r"lifetime: ([0-9]+) sec\r\r\n", server_output)
    objects = re.search(r"objects: ([\/0-9]+ ?(\([0-9.]+\))?, ?)+\r\r\n", server_output)
    return int(client_id), event, endpoint, version, binding, int(lifetime), objects.string


def test_registration_interface(lwm2mserver, lwm2mclient):
    """LightweightM2M-1.1-int-101
    Test that the Client registers with the Server."""

    lwm2mclient.waitfortext("STATE_READY")
    text = lwm2mserver.waitforpacket()
    client_id, event, endpoint, version, binding, lifetime, objects = \
        parse_client_registration(text)
    assert client_id == 0
    assert event == "registered"
    assert endpoint == "testlwm2mclient"
    assert version == "1.1"
    assert binding == "UDP"
    assert lifetime == 300
    assert "/1 (1.1)" in objects
    assert "/1/0" in objects
