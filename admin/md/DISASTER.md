# Disaster Scenarios

### Restarting each component 

#### Mysql DB

SSH into the AWS instance hosting the database.

    $ sudo service mysql restart

#### Website backend and Manger

SSH into the AWS instance hosting the website and manager.

    $ sudo service apache2 restart

#### Workers

On your local machine, assuming that your halite.ini is configured correctly and the database is up:

    $ cd admin
    $ python3 commandRunner.py 'cd Halite/worker; sudo ./stopWorkerScreen.sh; ./startWorkerScreen.sh'

Let's go over that last line. The `commandRunner.py` script uses ssh to run arbitrary bash commands on all of our workers. You pass it your desired bash command as a command line arguement (in this case `cd Halite/worker; ...`). The `sudo ./stopWorkerScreen.sh` command will kill all of the screens on a worker (and therefore the process that is running `worker.py`) and will stop and remove all docker containers on a worker. `./startWorkerScreen.sh` starts a detached screen and runs `python3 worker.py` in it.

***Note:*** This will have to be updated soon. AWS doesn't let you login as root over ssh.

### Bad bot submitted

### Account DDOS

### Renew/Reinstall SSL

Run:

    $ certbot-auto --apache -d halite.io -d www.halite.io
    
You will be presented with a couple of cursese menus. Renewing the certificate should be tried first. "Easy" (both http and https) should be picked instead of "Secure."

### Restarting from MySQL database backup

***These steps will delete all data in the production db***

Login to the backup server over SFTP using your favorite SFTP client ([Filezilla](https://filezilla-project.org/) is quite good). Navigate into the `/backup/db` folder. Grab the newest file (also the file that starts with the biggest number as these files are named by the timestamp when they were created).

Transfer this file to the home directory of the DB server directly or by downloading it to your local computer, logging into the DB server over SFTP, and then uploading it to the DB server.

SSH into the db server, and:

    $ echo "drop database Halite; create database Halite;" | mysql -u root -p
    $ mysql -u root -p Halite < NAME_OF_THE_SQL_FILE.sql
