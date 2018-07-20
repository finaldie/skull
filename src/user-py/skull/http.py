"""
Python Layer APIs for handling Http Request and Response (Based on WebOb)

For Request class: It help to parse the raw http request to a structured
  Request

For Response class: User use it to build a http response easily

In generally, we can setup a proxy/load balancer in front of skull, then use
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
         proxy_set_header X-Forwarded-For $remote_addr;
         proxy_pass http://backend;
     }

 ```
"""

from io import StringIO
from wsgiref import simple_server, util

from webob import Request as WebObRequest
from webob import Response as WebObResponse
import webob.request as WebObReqModule

class RequestIncomplete(Exception):
    def __init__(self, reason):
        self.message = reason

class Request(simple_server.WSGIRequestHandler):
    def __init__(self, data):
        self._env = None

        # Wrap input data into stringio
        self._raw_data = data
        self._input = StringIO(data)

        # Set up required members
        self.rfile  = self._input
        self.wfile  = StringIO() # for error msgs
        self.server = self
        self.base_environ    = {}
        self.client_address  = ['?', 80] # fake client address
        self.raw_requestline = self.rfile.readline()

    def parse(self):
        try:
            self.parse_request()

            # After parsing, we can setup the environ
            self._env = self.getEnv()
            self.request = WebObRequest(self._env)
        except Exception as e:
            raise e

        # Verify the request body is complete
        content_length = self.request.content_length
        if content_length is not None and content_length > 0:
            try:
                if len(self.request.body) != content_length:
                    raise RequestIncomplete('Request Body Incomplete')
            except WebObReqModule.DisconnectionError as de:
                raise RequestIncomplete('Request Body Incomplete: {}'.format(de))

        return self.request

    def getEnv(self):
        if self._env is not None:
            return self._env

        # Fill CGI required fields
        env = self._env = self.get_environ()

        # Fill WSGI required fields
        env['wsgi.input']        = self.rfile # point to body (The internal position of this IO is currently point to body)
        env['wsgi.errors']       = self.wfile
        env['wsgi.version']      = (1, 0)
        env['wsgi.run_once']     = False
        env['wsgi.url_scheme']   = util.guess_scheme(env)
        env['wsgi.multithread']  = True
        env['wsgi.multiprocess'] = False

        return env

    def address_string(self):
        """This is used for override the BaseHTTPRequestHandler.address_string
         Which will be calling the gethostbyaddr() blocking API (leading
         latency issue)
        """
        return 'unknow'


class Response(object):
    def __init__(self):
        self.response = WebObResponse()

    def getFullContent(self):
        return "HTTP/1.1 {}".format(self.response)

