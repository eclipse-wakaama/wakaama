"""helper functions for integration tests"""


def get_senml_json_record(parsed, urn, label):
    """helper function returning value of associated label

    example SenML input:

        [{'bn': '/1/', 'n': '0/0', 'v': 123}, {'n': '0/1', 'v': 300},
        {'n': '0/2', 'v': 0}, {'n': '0/3', 'v': 0}, {'n': '0/5', 'v': 0},
        {'n': '0/6', 'vb': False}, {'n': '0/7', 'vs': 'U'}]"""

    return next(item for item in parsed if item["n"] == urn)[label]
