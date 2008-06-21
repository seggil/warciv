#!/bin/bash

# A simple bash script to block IP traffic from TOR exit nodes.
# written by Andrew Vetlugin (antrew at gmail com)

wget='/usr/bin/wget'
iptables='/sbin/iptables'
url='https://torstat.xenobite.eu/export/tor_exitnodes.csv'

iptables_target='DROP'
#iptables_target='REJECT'

# Quick guide:

# 1. add a separate chain for a list of TOR exit nodes
# (this should be done by hand once)
# iptables -N TOR_EXIT

# 2. add a rule to INPUT chain
# Note: if you want to be able to connect to any TOR exit node yourself
# (e.g., if $url is a exit node you should be able to fetch a list of exit
# nodes from it) then you should add this rule AFTER accepting established
# and related connections)
# iptables -A INPUT -j TOR_EXIT

# 3. add this script to crontab (I think 10-20 minutes interval should be OK)

# flush chain
$iptables -F TOR_EXIT

# add TOR exit nodes to TOR_EXIT chain with $iptables_target rule
for node in `$wget -q --no-check-certificate -O - $url | sort | uniq` ; do
	$iptables -A TOR_EXIT -s $node -j $iptables_target
done

# return to parent chain if the source is not TOR exit node
$iptables -A TOR_EXIT -j RETURN
