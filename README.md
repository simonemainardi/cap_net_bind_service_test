# cap_net_bind_service_test
The aim of this project is to demonstrate how to bind non-root processes to privileged ports (<1024) on Linux

Compile: `gcc cap_net_bind_service_test.c -lcap -o cap_net_bind_service_test`
Run: `./cap_net_bind_service_test`

Depends on `libcap-dev`.