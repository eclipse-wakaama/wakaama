"""Wakaama integration tests (pytest)"""
import re
import json
import pytest
from helpers.helpers import get_senml_json_record
from helpers.helpers import get_json_record
from helpers.helpers import get_tlv_record

# pylint: disable=unused-argument

@pytest.mark.specialbuild
def test_querying_basic_information_in_tlv_format(lwm2mserver, lwm2mclient):
    """LightweightM2M-1.1-int-203
    Querying the Resources Values of Device Object ID:3 on the Client
    Resources Values of Device Object ID:3"""

    # Test Procedure 1
    assert lwm2mserver.commandresponse("read 0 /3/0", "OK")
    text = lwm2mserver.waitforpacket()
    # Pass-Criteria A
    assert text.find("COAP_205_CONTENT") > 0
    assert text.find("lwm2m+tlv") > 0
    assert get_tlv_record(text, 0) == "Open Mobile Alliance"
    assert get_tlv_record(text, 1) == "Lightweight M2M Client"
    assert get_tlv_record(text, 2) == "345000123"
    assert get_tlv_record(text, 3) == "1.0"
    assert get_tlv_record(text, 11) == "." # not printable char
    assert get_tlv_record(text, 16) == "U"


@pytest.mark.specialbuild
def test_querying_basic_information_in_json_format(lwm2mserver, lwm2mclient):
    """LightweightM2M-1.1-int-204
    Querying the Resources Values of Device Object ID:3 on the Client using
    JSON data format"""

    # Test Procedure 1
    assert lwm2mserver.commandresponse("read 0 /3/0", "OK")
    text = lwm2mserver.waitforpacket()
    # Pass-Criteria A
    assert text.find("COAP_205_CONTENT") > 0
    assert text.find("lwm2m+json") > 0
    packet = re.findall(r"({.*})", text)
    parsed = json.loads("["+packet[0]+"]")[0]
    assert parsed['bn'] == "/3/0/"
    assert get_json_record(parsed, "0", "sv") == "Open Mobile Alliance"
    assert get_json_record(parsed, "1", "sv") == "Lightweight M2M Client"
    assert get_json_record(parsed, "2", "sv") == "345000123"
    assert get_json_record(parsed, "3", "sv") == "1.0"
    assert get_json_record(parsed, "11/0", "v") == 0
    assert get_json_record(parsed, "16", "sv") == "U"


def test_read_on_object(lwm2mserver, lwm2mclient):
    """LightweightM2M-1.1-int-222
    Purpose of this test is to show conformance with the LwM2M Read
    operation on whole Object"""

    lwm2mclient.waitfortext("STATE_READY")
    # Test Procedure 1
    assert lwm2mserver.commandresponse("read 0 /1", "OK")
    text = lwm2mserver.waitforpacket()
    # Pass-Criteria A
    assert text.find("COAP_205_CONTENT") > 0
    packet = re.findall(r"(\[.*\])", text)
    parsed = json.loads(packet[1])
    assert get_senml_json_record(parsed, "0/0", "v") == 123
    assert get_senml_json_record(parsed, "0/0", "bn") == "/1/"
    assert get_senml_json_record(parsed, "0/1", "v") == 300
    assert get_senml_json_record(parsed, "0/2", "v") == 0
    assert get_senml_json_record(parsed, "0/3", "v") == 0
    assert get_senml_json_record(parsed, "0/5", "v") == 0
    assert get_senml_json_record(parsed, "0/6", "vb") is False
    assert get_senml_json_record(parsed, "0/7", "vs") == 'U'
    # Test Procedure 2
    assert lwm2mserver.commandresponse("read 0 /3", "OK")
    text = lwm2mserver.waitforpacket()
    # Pass-Criteria B
    assert text.find("COAP_205_CONTENT") > 0
    packet = re.findall(r"(\[.*\])", text)
    parsed = json.loads(packet[1])
    # assert that only instance 0 is of object 3 is populated
    for item in parsed:
        assert item["n"].startswith("0/")
    assert get_senml_json_record(parsed, "0/0", "vs") == "Open Mobile Alliance"
    assert get_senml_json_record(parsed, "0/0", "bn") == "/3/"
    assert get_senml_json_record(parsed, "0/2", "vs") == "345000123"
    assert get_senml_json_record(parsed, "0/3", "vs") == "1.0"
    assert get_senml_json_record(parsed, "0/6/0", "v") == 1
    assert get_senml_json_record(parsed, "0/6/1", "v") == 5
    assert get_senml_json_record(parsed, "0/7/0", "v") == 3800
    assert get_senml_json_record(parsed, "0/7/1", "v") == 5000
    assert get_senml_json_record(parsed, "0/8/0", "v") == 125
    assert get_senml_json_record(parsed, "0/8/1", "v") == 900
    assert get_senml_json_record(parsed, "0/9", "v") == 100
    assert get_senml_json_record(parsed, "0/10", "v") == 15
    assert get_senml_json_record(parsed, "0/11/0", "v") == 0
    assert get_senml_json_record(parsed, "0/13", "v") > 0    # current time
    assert get_senml_json_record(parsed, "0/14", "vs") == "+01:00"
    assert get_senml_json_record(parsed, "0/15", "vs") == "Europe/Berlin"
    assert get_senml_json_record(parsed, "0/16", "vs") == "U"
