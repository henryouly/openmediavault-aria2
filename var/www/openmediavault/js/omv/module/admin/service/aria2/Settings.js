Ext.define("OMV.module.admin.service.aria2.Settings", {
    extend: "OMV.workspace.form.Panel",
    
    // This path tells which RPC module and methods this panel will call to get 
    // and fetch its form values.
    rpcService: "Aria2",
    rpcGetMethod: "getSettings",
    rpcSetMethod: "setSettings",
    
    // getFormItems is a method which is automatically called in the 
    // instantiation of the panel. This method returns all fields for 
    // the panel.
    getFormItems: function() {
        return [{
            // xtype defines the type of this entry. Some different types
            // is: fieldset, checkbox, textfield and numberfield. 
            xtype: "fieldset",
            title: _("General"),
            fieldDefaults: {
                labelSeparator: ""
            },
            // The items array contains items inside the fieldset xtype.
            items: [{
                xtype: "checkbox",
                // The name option is sent together with is value to RPC
                // and is also used when fetching from the RPC.
                name: "enable",
                fieldLabel: _("Enable"),
                // checked sets the default value of a checkbox.
                checked: false
            },{
				xtype: "sharedfoldercombo",
				name: "dir",
				fieldLabel: _("Data directory"),
				allowNone: true,
				plugins: [{
					ptype: "fieldinfo",
					text: _("The location where aria2 stores its files.")
				}]
			},{
                xtype: "numberfield",
                name: "max-connection-per-server",
                fieldLabel: _("Max. connections per host"),
                minValue: 0,
                maxValue: 10,
                allowDecimals: false,
                allowBlank: true,
                value: 5,
				plugins: [{
					ptype: "fieldinfo",
					text: _("Maximum number of connections per IP (0 = unlimited).")
				}]
			},{
                xtype: "numberfield",
                name: "max-concurrent-downloads",
                fieldLabel: _("Max. clients"),
                minValue: 0,
                maxValue: 10,
                allowDecimals: false,
                allowBlank: true,
                value: 3,
				plugins: [{
					ptype: "fieldinfo",
					text: _("Maximum number of simultaneous clients.")
				}]
			},{
                xtype: "numberfield",
                name: "split",
                fieldLabel: _("Split parts"),
                minValue: 0,
                maxValue: 20,
                allowDecimals: false,
                allowBlank: true,
                value: 5,
				plugins: [{
					ptype: "fieldinfo",
					text: _("The number of parts to split.")
				}]
			},{
                xtype: "numberfield",
                name: "max-overall-download-limit",
                fieldLabel: _("Maximum download rate (KiB/s)"),
                minValue: 0,
                allowDecimals: false,
                allowBlank: true,
                value: 0,
				plugins: [{
					ptype: "fieldinfo",
					text: _("0 KiB/s means unlimited.")
				}]
            }]
        }];
    }
});

// Register a panel into the GUI.
OMV.WorkspaceManager.registerPanel({
    id: "settings", 
    path: "/service/aria2", 
    text: _("Settings"), 
    position: 10,
    className: "OMV.module.admin.service.aria2.Settings" 
});
