#!/bin/sh

cp /var/www/openmediavault/js/omv/module/admin/service/aria2/* var/www/openmediavault/js/omv/module/admin/service/aria2
cp /usr/share/openmediavault/engined/rpc/aria2.inc usr/share/openmediavault/engined/rpc/aria2.inc
cp /usr/share/openmediavault/engined/module/aria2.inc usr/share/openmediavault/engined/module/aria2.inc
cp /usr/share/openmediavault/mkconf/aria2 usr/share/openmediavault/mkconf/aria2
cp /etc/init.d/aria2 etc/init.d/aria2
