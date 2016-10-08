"""
Python Layer APIs for handling Http Request and Response (Based on WebOb)

For Request class: It help to parse the raw http request to a structured
  Request

For Response class: User use it to build a http response easily

In generally, you can setup a proxy/load balancer in front of skull, then use
 these APIs to parse http request and build http response

 For example, add the following into /etc/nginx/nginx.conf
 ```
 upstream backend {
	server 127.0.0.1:7758;
 }
 ...

 server {
     ...
     location /test {
         proxy_pass http://backend;
     }

 ```
"""

import StringIO
from wsgiref import simple_server, util

from webob import Request as WebObRequest
from webob import Response as WebObResponse

class Request(simple_server.WSGIRequestHandler):
    def __init__(self, data):
        # Wrap input data into stringio
        self._raw_data = data
        self._input = StringIO.StringIO(data)

        # Set up required members
        self.rfile = self._input
        self.wfile = StringIO.StringIO() # for error msgs
        self.server = self
        self.base_environ = {}
        self.client_address = ['?', 80]
        self.raw_requestline = self.rfile.readline()

    def parse(self):
        try:
            self.parse_request()
            self.request = WebObRequest(self._getEnv())
        except Exception as e:
            raise e

        return self.request

    def _getEnv(self):
        env = self.get_environ()
        util.setup_testing_defaults(env)
        env['wsgi.input'] = self.rfile
        return env

class Response(object):
    def __init__(self):
        self.response = WebObResponse()

    def getFullContent(self):
        return "HTTP/1.1 {}".format(self.response)

