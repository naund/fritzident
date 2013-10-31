#
# Regular cron jobs for the fritzident package
#
0 4	* * *	root	[ -x /usr/bin/fritzident_maintenance ] && /usr/bin/fritzident_maintenance
