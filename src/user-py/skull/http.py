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
    """
    RequestIncomplete(Exception) - Customized exception
    """
    def __init__(self, reason):
        super(RequestIncomplete, self).__init__(reason)
        self.message = reason

class Request(simple_server.WSGIRequestHandler):
    """
    Http Request class
    """

    def __init__(self, data):
        """
        __init__ - constructor
        """
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
        """
        parse(self) - parse http request
        """

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
        """
        getEnv(self) - Return env table
        """

        if self._env is not None:
            return self._env

        # Fill CGI required fields
        env = self._env = self.get_environ()

        # Fill WSGI required fields
        ## 'input' point to body (The internal position of this IO is currently
        ## point to body)
        env['wsgi.input']        = self.rfile
        env['wsgi.errors']       = self.wfile
        env['wsgi.version']      = (1, 0)
        env['wsgi.run_once']     = False
        env['wsgi.url_scheme']   = util.guess_scheme(env)
        env['wsgi.multithread']  = True
        env['wsgi.multiprocess'] = False

        return env

    def address_string(self):
        """This is used for override the BaseHTTPRequestHandler.address_string
         Which will call the gethostbyaddr() blocking API (leading
         latency issue)
        """
        return 'unknow'


class Response():
    """
    Http Reponse class
    """

    def __init__(self):
        """
        __init__ - constructor
        """
        self.response = WebObResponse()

    def getFullContent(self):
        """
        getFullContent - Return the whole http response content
        """
        return "HTTP/1.1 {}".format(self.response)

