'''Wakaama integration tests (pytest)'''
import pytest


@pytest.mark.client_args("-b -n apa")
def test_client_initiated_bootstrap(lwm2mbootstrapserver, lwm2mclient):
    """LightweightM2M-1.1-int-0
    Test the Client capability to connect the Bootstrap Server according to the
    Client Initiated Bootstrap Mode"""

    # check that client attempts to bootstrap
    assert lwm2mclient.waitfortext("STATE_BOOTSTRAPPING")

    # Test Procedure 1
    # bootstrap request is triggered by command line option
    assert lwm2mbootstrapserver.waitfortext('Bootstrap request from "apa"\r\r\n')

    # Pass-Criteria A
    # should check for "2.04" changed but informarmation not available at CLI.
    # Just check that we got packet (of correct size)
    assert lwm2mclient.waitfortext("8 bytes received from")

    # check that DELETE /0 is sent and ack:ed (not in TC spec)
    assert lwm2mbootstrapserver.waitfortext('Sending DELETE /0 to "apa" OK.')
    assert lwm2mbootstrapserver.waitfortext('Received status 2.02 (COAP_202_DELETED) for URI /0 from endpoint apa.')

    # check that DELETE /1 is sent and ack:ed (not in TC spec)
    assert lwm2mbootstrapserver.waitfortext('Sending DELETE /1 to "apa" OK.')
    assert lwm2mbootstrapserver.waitfortext('Received status 2.02 (COAP_202_DELETED) for URI /1 from endpoint apa.')

    # check that bootstrap write operations are sent (not checking data)
    assert lwm2mbootstrapserver.waitfortext('Sending WRITE /0/1 to "apa" OK.')
    # Pass-Criteria C
    # check that bootstrap server received success ACK
    assert lwm2mbootstrapserver.waitfortext('Received status 2.04 (COAP_204_CHANGED) for URI /0/1 from endpoint apa.')

    # check that bootstrap write operations are sent (not checking data)
    assert lwm2mbootstrapserver.waitfortext('Sending WRITE /1/1 to "apa" OK.')
    # Pass-Criteria C
    # check that bootstrap server received success ACK
    assert lwm2mbootstrapserver.waitfortext('Received status 2.04 (COAP_204_CHANGED) for URI /1/1 from endpoint apa.')

    # Pass-Criteria D
    # check bootstrap discover ACK
    assert lwm2mbootstrapserver.waitfortext('Received status 2.05 (COAP_205_CONTENT) for URI / from endpoint apa.')

    # Pass-Criteria E
    # check that bootstrap server receives a successful ACK for bootstrap finish
    assert lwm2mbootstrapserver.waitfortext('Sending BOOTSTRAP FINISH  to "apa" OK.')
    assert lwm2mbootstrapserver.waitfortext('Received status 2.04 (COAP_204_CHANGED) for URI / from endpoint apa.')
