## Cannot connect to RPC endpoint

Check if your local nodapifiny is running by visiting the following URL in your browser:

```shell

curl http://localhost:8888/v1/chain/get_info

```

If you are trying to connect a remote nodapifiny API endpoint, try to visit the API endpoint with the following suffix:

```shell
http://API_ENDPOINT:PORT/v1/chain/get_info
```

Replace API_ENDPOINT and PORT with your remote nodapifiny API endpoint detail

## “Missing Authorizations"

That means you are not using the required authorizations. Most likely you are not using correct APIFINY account or permission level to sign the transaction
