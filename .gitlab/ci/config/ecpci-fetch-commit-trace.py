#!/usr/bin/env python3

import json
import ssl
import sys
import urllib.request


class ecpci_url_reader:
    def __init__(self, base_url, token):
        self.base_url = base_url
        self.token = token

    def to_string(self, url):
        opener = urllib.request.build_opener(
                     urllib.request.HTTPSHandler(
                         context=ssl._create_unverified_context()))
        opener.addheaders = [('PRIVATE-TOKEN', token)]
        return opener.open(base_url + url).read().decode('utf-8')

    def to_json(self, url):
        return json.loads(self.to_string(url))


base_url = sys.argv[1]
commit = sys.argv[2]
token = sys.argv[3]

handler = ecpci_url_reader(base_url, token)

commit_info = handler.to_json("/repository/commits/" + commit)
last_pipeline_id = str(commit_info['last_pipeline']['id'])

jobs = handler.to_json("/pipelines/" + last_pipeline_id + "/jobs")
build_job_id = str(jobs[1]['id'])
test_job_id = str(jobs[0]['id'])

print("ECPCITEST BUILD OUTPUT================================================")
print(handler.to_string("/jobs/" + build_job_id + "/trace"))
print("ECPCITEST BUILD END===================================================")

print("ECPCITEST TEST OUTPUT=================================================")
print(handler.to_string("/jobs/" + test_job_id + "/trace"))
print("ECPCITEST TEST OUTPUT END=============================================")
