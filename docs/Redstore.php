<?php

/*
    Class to talk to RedStore from PHP.
    Copyright (C) 2010 Nicholas J Humfrey <njh@aelius.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


class RedStore {

    private $_endpoint = null;
    private $_curl = null;
    
    public function __construct($endpoint) {
        $this->_endpoint = $endpoint;
        $this->_curl = curl_init();
        curl_setopt($this->_curl, CURLOPT_TIMEOUT, 10);
        curl_setopt($this->_curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
        curl_setopt($this->_curl, CURLOPT_USERAGENT, 'RedStorePHP/0.1');
        curl_setopt($this->_curl, CURLOPT_RETURNTRANSFER, 1);
    }
    
    function __destruct() {
        curl_close($this->_curl);
    }
    
    public function load($uri, $base_uri=null, $graph=null) {
        $postfields = "uri=".urlencode($uri);
        if ($base_uri)
            $postfields = "&base-uri=".urlencode($base_uri);
        if ($graph)
            $postfields = "&graph=".urlencode($graph);

        curl_setopt($this->_curl, CURLOPT_URL, $this->_endpoint."load");
        curl_setopt($this->_curl, CURLOPT_POST, 1);
        curl_setopt($this->_curl, CURLOPT_CUSTOMREQUEST, "POST");
        curl_setopt($this->_curl, CURLOPT_HTTPHEADER, array());
        curl_setopt($this->_curl, CURLOPT_POSTFIELDS, $postfields);
        curl_exec($this->_curl);
        
        return $this->is_success();
    }
    
    public function delete($uri) {
        curl_setopt($this->_curl, CURLOPT_URL, $this->graph_uri($uri));
        curl_setopt($this->_curl, CURLOPT_POST, 1);
        curl_setopt($this->_curl, CURLOPT_CUSTOMREQUEST, "DELETE");
        curl_setopt($this->_curl, CURLOPT_HTTPHEADER, array());
        curl_setopt($this->_curl, CURLOPT_POSTFIELDS, null);
        curl_exec($this->_curl);
        
        return $this->is_success();
    }
    
    public function put($data, $uri=null, $format='application/rdf+xml') {
        if (!$uri and is_object($data) and method_exists($data, 'getUri')) {
            $uri = $data->getUri();
        }

        if (is_object($data) and method_exists($data, 'serialise')) {
            $data = $data->serialise($format);
        }
    
        curl_setopt($this->_curl, CURLOPT_URL, $this->graph_uri($uri));
        curl_setopt($this->_curl, CURLOPT_POST, 1);
        curl_setopt($this->_curl, CURLOPT_CUSTOMREQUEST, "PUT");
        curl_setopt($this->_curl, CURLOPT_HTTPHEADER, array(
            "Content-Type: ".$format,
            "Content-Length: ".strlen($data)
        ));
        curl_setopt($this->_curl, CURLOPT_POSTFIELDS, $data);
        curl_exec($this->_curl);
        
        return $this->is_success();
    }
    
    public function get($uri, $format='application/rdf+xml') {
        curl_setopt($this->_curl, CURLOPT_URL, $this->graph_uri($uri));
        curl_setopt($this->_curl, CURLOPT_POST, 0);
        curl_setopt($this->_curl, CURLOPT_CUSTOMREQUEST, "GET");
        curl_setopt($this->_curl, CURLOPT_HTTPHEADER, array("Accept: ".$format));
        curl_setopt($this->_curl, CURLOPT_POSTFIELDS, null);
        return curl_exec($this->_curl);
    }
    
    public function is_success() {
        return (curl_getinfo($this->_curl, CURLINFO_HTTP_CODE) == 200);
    }
    
    public function graph_uri($uri) {
        return $this->_endpoint."data/".urlencode($uri);
    }
}


?>
