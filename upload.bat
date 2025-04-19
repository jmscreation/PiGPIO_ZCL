@echo off

call ssh_creds.bat

set OUTPUT=program

echo Uploading To Pi...
	:: Remote Update - Send Updates To Remote Server For Update
	scp -i %SSH_KEY% .\%OUTPUT% %PI_USERNAME%@%PI_HOSTNAME%:~/program
	scp -i %SSH_KEY% .\config.conf %PI_USERNAME%@%PI_HOSTNAME%:~/config.conf