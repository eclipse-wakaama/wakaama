"""Wakaama integration tests (pytest)"""
import re
import pytest


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
        objects: /1 (1.1), /1/0, /2/0, /3/0, /4/0, /5/0, /6/0, /7/0, <added linebreak>
                 /31024 (1.0), /31024/10, /31024/11, /31024/12,
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


@pytest.mark.slow
def test_registration_information_update(lwm2mserver, lwm2mclient):
    """LightweightM2M-1.1-int-102
    Test that the client updates the registration information on the Server."""

    lwm2mclient.waitfortext("STATE_READY")
    # Test Procedure 1
    assert lwm2mserver.commandresponse("write 0 /1/0/1 20", "OK")
    text = lwm2mserver.waitforpacket()
    # Pass-Criteria A
    assert text.find("COAP_204_CHANGED") > 0
    # Test Procedure 2
    text = lwm2mserver.waitfortime(20)
    # Pass-Criteria B, D (C not observable)
    assert text.find("lifetime: 300 sec") > 0 # NOTE: this should be 20 sec?
    # expect update response immediately and again after every half "lifetime" (10s)
    assert 2 <= text.count("Client #0 updated.") <= 3


@pytest.mark.slow
@pytest.mark.client_args("-t 60")
def test_discarded_register_update(lwm2mserver, lwm2mclient):
    """LightweightM2M-1.1-int-105
    Test if Server replies correctly (with error message) upon receiving an
    unexpected Register Update (e.g. after Lifetime expires). Test if Client
    respects the error message and fallbacks full registration (re-register).

    Test only partially implemented due to limitations (see comments)"""

    lwm2mclient.waitfortext("STATE_READY")
    # Test Procedure 1
    # server learns client lifetime when client registers
    # a server write to /1/x/1 does not affect the servers knowledge about
    # client lifetime therefore a higher value than the inital registration lifetime
    # is written to this resource to provoke the server to drop the client
    text = lwm2mserver.waitfortime(60)
    # Pass-Criteria A
    assert text.find("Client #0 updated.")
    # Test Procedure 2
    # Client reports 60s lifetime to server
    # Server changes client lifetime to 180
    #   => server will drop client before an update is received
    assert lwm2mserver.commandresponse("write 0 /1/0/1 180", "OK")
    # ensure client is dropped >60s (in practice >30s)
    text = lwm2mserver.waitfortime(70)
    # Pass-Criteria B
    assert text.find("Client #0 unregistered.") > 0
    # Test Procedure 3
    # wait long enough to client to send register update (180s total)
    # Pass-Criteria C
    # only in debug build it is possible to detect "4.04" from printouts
    # Pass-Criteria D
    # Client should perform full registration according as in int-101
    # this is not implemented, client exits with "STATE_BOOTSTRAP_REQUIRED"
