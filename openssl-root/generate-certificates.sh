#!/bin/bash
# https://stackoverflow.com/a/21340898
openssl req -x509 -config openssl-ca.cnf -days 3650 -newkey rsa:4096 -sha256 -nodes -out cacert.pem -outform PEM
openssl req -config openssl-server.cnf -newkey rsa:2048 -sha256 -nodes -out servercert.csr -outform PEM
openssl ca -config openssl-ca.cnf -policy signing_policy -extensions signing_req -out cert.pem -infiles servercert.csr
