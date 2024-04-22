#!/bin/bash

##########
# Checks #
##########
if [ ! -f ../halite.ini ]; then
    echo "FAIL\nCannot install worker until a halite.ini file is created."
    return
fi

##################
# Compiler tools #
##################
add-apt-repository -y ppa:ubuntu-toolchain-r/test 
apt-get -qq update
apt-get -qq install g++-4.9

apt-get install -y curl build-essential python3 default-jdk
curl -sSf https://static.rust-lang.org/rustup.sh | sh

##########
# Worker #
##########
apt-get install -y zip python3-pip
pip3 install requests
pip3 install zip

################
# Docker SETUP #
################
apt-key adv --keyserver hkp://p80.pool.sks-keyservers.net:80 --recv-keys 58118E89F3A912897C070ADBF76221572C52609D
echo "deb https://apt.dockerproject.org/repo ubuntu-trusty main" | sudo tee /etc/apt/sources.list.d/docker.list
apt-get update
apt-get install -y docker-engine

# Use devicemapper storage driver, so that we can limit storage space of containers
service docker stop
dockerd --storage-driver=devicemapper &
service docker start

./buildDocker.sh

###############
# Swap Memory #
###############
# Replace line 12
sed -i '12s/.*/GRUB_CMDLINE_LINUX="cgroup_enable=memory swapaccount=1"/' /etc/default/grub

update-grub

###########
# API Key #
###########
if [ "$#" -eq 1 ]; then
    python3 changeAPIKey.py $1
fi

#################
# Force g++ 4.9 #
#################
update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-4.9 10

echo "A reboot is required to complete this installation!"

