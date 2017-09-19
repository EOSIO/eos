# EOS.IO testnet on Ubuntu 16.04 LTS VM

<a href="https://portal.azure.com/#create/Microsoft.Template/uri/https%3A%2F%2Fraw.githubusercontent.com%2FryanRfox%2Feos%2Fmaster%2FAzure%2Feos-ubuntu-vm%2Fazuredeploy.json" target="_blank"><img src="http://azuredeploy.net/deploybutton.png"/></a>
<a href="http://armviz.io/#/?load=https%3A%2F%2Fraw.githubusercontent.com%2FryanRfox%2Feos%2Fmaster%2FAzure%2Feos-ubuntu-vm%2Fazuredeploy.json" target="_blank">
    <img src="http://armviz.io/visualizebutton.png"/>
</a>

This template uses the Azure Linux CustomScript extension to deploy an EOS.IO test network. The deployment template creates an Ubuntu 16.04 LTS VM, installs the EOS.IO software, configures eosd to run as a service, then adds 21 default block producing nodes to bootstrap the local test network. 