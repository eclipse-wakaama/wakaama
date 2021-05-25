"""Wakaama integration tests (pytest)"""
import re
import json
import pytest
from helpers.helpers import get_senml_json_record


# pylint: disable=unused-argument

@pytest.mark.slow
def test_observation_notification(lwm2mserver, lwm2mclient):
    """LightweightM2M-1.1-int-301
    Sending the observation policy to the device."""

    # Test Procedure 1
    assert lwm2mserver.commandresponse("read 0 /3/0/6", "OK")
    text = lwm2mserver.waitforpacket()
    assert text.find("COAP_205_CONTENT") > 0
    packet = re.findall(r"(\[.*\])", text)
    parsed = json.loads(packet[1])
    assert next(item for item in parsed if item["n"] == "0")["v"] == 1 # Internal battery
    assert next(item for item in parsed if item["n"] == "1")["v"] == 5 # USB
    # Test Procedure 2
    assert lwm2mserver.commandresponse("time 0 /3/0/7 5 15", "OK")
    assert lwm2mserver.commandresponse("time 0 /3/0/8 10 20", "OK")
    # Test Procedure 3
    assert lwm2mserver.commandresponse("observe 0 /3/0/7", "OK")
    assert lwm2mserver.commandresponse("observe 0 /3/0/8", "OK") #consumes first observe from /3/0/7
    text = lwm2mserver.waitforpacket() # consumes ACK and piggy-backed first observe from /3/0/8
    text = lwm2mserver.waitfortime(20) # wait for max "pmax"
    # Pass-Criteria A
    assert 0 < text.count("Notify from client #0 /3/0/7") <= 4
    assert 0 < text.count("Notify from client #0 /3/0/8") <= 2
    packets = re.findall(r"(\[\{.*\}\])", text)
    for packet in packets:
        parsed = json.loads(packet)
        print(get_senml_json_record(parsed, "0", "bn"))
        if get_senml_json_record(parsed, "0", "bn") == "/3/0/7/":
            assert get_senml_json_record(parsed, "0", "v") == 3800
            assert get_senml_json_record(parsed, "1", "v") == 5000
        elif get_senml_json_record(parsed, "0", "bn") == "/3/0/8/":
            assert get_senml_json_record(parsed, "0", "v") == 125
            assert get_senml_json_record(parsed, "1", "v") == 900
        else:
            assert False


@pytest.mark.slow
def test_cancel_observations_using_reset(lwm2mserver, lwm2mclient):
    """LightweightM2M-1.1-int-302
    Cancel the Observation relationship by sending “Cancel Observation”
    operation."""
    # Test Procedure 1
    assert lwm2mserver.commandresponse("time 0 /3/0/7 3 6", "OK") # faster timeout vs int-301
    assert lwm2mserver.commandresponse("time 0 /3/0/8 6 7", "OK")
    assert lwm2mserver.commandresponse("observe 0 /3/0/7", "OK")
    assert lwm2mserver.commandresponse("observe 0 /3/0/8", "OK")
    text = lwm2mserver.waitfortime(15)
    # Pass-Criteria A
    # Make sure that notifications are sent
    assert text.count("Notify from client #0 /3/0/7") >= 2
    assert text.count("Notify from client #0 /3/0/8") >= 2
    # Test Procedure 2
    assert lwm2mserver.commandresponse("cancel 0 /3/0/7", "OK")
    text = lwm2mserver.waitfortime(15)
    # Pass-Criteria B
    assert text.count("Notify from client #0 /3/0/7") == 0
    assert text.count("Notify from client #0 /3/0/8") >= 2
    # Test Procedure 3, 4
    assert lwm2mserver.commandresponse("cancel 0 /3/0/8", "OK")
    text = lwm2mserver.waitfortime(15)
    # Pass-Criteria B cont.
    assert text.count("Notify from client #0 /3/0/7") == 0
    assert text.count("Notify from client #0 /3/0/8") == 0
