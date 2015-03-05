Ext.define("OMV.module.admin.service.aria2.Status", {
    extend: "OMV.workspace.grid.Panel",
    requires: [
        "OMV.Rpc",
        "OMV.data.Store",
        "OMV.data.Model",
        "OMV.data.proxy.Rpc"
    ],
    autoReload: true,
    hidePagingToolbar: false,
    hideEditButton: true,
    rememberSelected: true,
    disableLoadMaskOnLoad: true,
    reloadInterval: 10000,

    pauseButtonText: _("Pause"),
    resumeButtonText: _("Resume"),
    
    columns: [{
        text: _("File name"),
        sortable: true,
        dataIndex: "filename",
        stateId: "filename",
        flex: 1
    },{
        text: _("Status"),
        sortable: true,
        dataIndex: "status",
        stateId: "status",
        width: 80,
        resizable: false,
        align: "center"
    },{
        text: _("Percent"),
        sortable: false,
        dataIndex: "percent",
        stateId: "percent",
        width: 80,
        resizable: false,
        align: "center"
    },{
        text: _("Speed"),
        sortable: false,
        dataIndex: "downloadSpeed",
        stateId: "downloadSpeed",
        width: 80,
        resizable: false,
        align: "center"

    },{
        text: _("ETA"),
        sortable: false,
        dataIndex: "eta",
        stateId: "eta",
        width: 80,
        resizable: false,
        align: "center"
    }],

    initComponent: function() {
        var me = this;
        Ext.apply(me, {
            store: Ext.create("OMV.data.Store", {
                autoLoad: true,
                model: OMV.data.Model.createImplicit({
                    idProperty: "uuid",
                    fields: [
                        { name: "id", type: "string" },
                        { name: "filename", type: "string" },
                        { name: "status", type: "string" },
                        { name: "percent", type: "string" },
                        { name: "eta", type: "string" },
                        { name: "downloadSpeed", type: "string" }
                    ]
                }),
                proxy: {
                    type: "rpc",
                    rpcData: {
                        service: "Aria2",
                        method: "getTaskList"
                    }
                }
            })
        });
        me.callParent(arguments);
    },

    getTopToolbarItems: function() {
        var items = this.callParent(arguments);

        Ext.Array.push(items, [{
            id: this.getId() + "-pause",
            xtype: "button",
            text: this.pauseButtonText,
            icon: "images/pause.png",
            iconCls: Ext.baseCSSPrefix + "btn-icon-16x16",
            handler: Ext.Function.bind(this.onPauseButton, this),
            disabled: true,
            scope: this,
            selectionConfig: {
                minSelections: 1,
                maxSelections: 1,
                enabledFn: this.setStatusButtonEnabled
            },
            action: "pause"
        },{
            id: this.getId() + "-resume",
            xtype: "button",
            text: this.resumeButtonText,
            icon: "images/play.png",
            iconCls: Ext.baseCSSPrefix + "btn-icon-16x16",
            handler: Ext.Function.bind(this.onResumeButton, this),
            disabled: true,
            scope: this,
            selectionConfig: {
                minSelections: 1,
                maxSelections: 1,
                enabledFn: this.setStatusButtonEnabled
            },
            action: "resume"
        }]);

        return items;
    },

	onAddButton: function() {
		Ext.create("OMV.module.admin.service.aria2.AddTask", {
			title: _("Add download task"),
			listeners: {
				scope: this,
				submit: function() {
					this.doReload();
				}
			}
		}).show();
	},

    onDeleteButton: function() {
        var records = this.getSelection();
		Ext.create("OMV.module.admin.service.aria2.DeleteTask", {
            listeners: {
                scope: this,
                submit: function(id, values) {
                    // Add delete_local_data to each item
                    Ext.Array.forEach(records, function(record) {
                        record.delete_local_data = values.delete_local_data;
                    });

                    this.startDeletion(records);
                }
            }
        }).show();
    },

    doDeletion: function(record) {
        console.debug('id = ' + record.get("id"));
        OMV.Rpc.request({
            scope: this,
            callback: this.onDeletion,
            rpcData: {
                service: "Aria2",
                method: "deleteTask",
                params: {
                    id: record.get("id"),
                    delete_local_data: record.delete_local_data
                }
            }
        });
    },

    setStatusButtonEnabled: function(button, records) {
        var record = records[0];
        var status = record.get("status");
        if (status === "pause" && button.action === "resume") {
            return true;
        }
        if (status === "active" && button.action === "pause") {
            return true;
        }
        return false;
    },

    onPauseButton: function() {
    },

    onResumeButton: function() {
    }
});

/**
 * @class OMV.module.admin.service.aria2.AddTask
 * @derived OMV.workspace.window.Form
 */
Ext.define("OMV.module.admin.service.aria2.AddTask", {
	extend: "OMV.workspace.window.Form",

	rpcService: "Aria2",
	rpcSetMethod: "addTask",
    hideResetButton: true,
    width: 500,
    title: _("Add download by URL"),
    okButtonText: _("OK"),
    submitMsg: _("Adding download ..."),

	getFormItems: function() {
		return [{
			xtype: "textfield",
			name: "url",
			fieldLabel: _("URL"),
            allowBlank: false
		},{
            xtype: "checkbox",
            name: "start_download",
            fieldLabel: _("Start download"),
            checked: true
        }];
	},

    onOkButton: function() {
        if (this.isValid()) {
            this.doSubmit();
        }
    }
});

/**
 * @class OMV.module.admin.service.aria2.DeleteTask
 * @derived OMV.workspace.window.Form
 */
Ext.define("OMV.module.admin.service.aria2.DeleteTask", {
	extend: "OMV.workspace.window.Form",

    hideResetButton: true,
    width: 500,
    title: _("Delete download"),
    okButtonText: _("OK"),
    submitMsg: _("Deleting download ..."),
    mode: "local",

	getFormItems: function() {
		return [{
			xtype: "checkbox",
			name: "delete_local_data",
			fieldLabel: _("Delete Local Data"),
            checked: false
        }];
	},

    onOkButton: function() {
        this.doSubmit();
    }
});

OMV.WorkspaceManager.registerPanel({
    id: "status",
    path: "/service/aria2",
    text: _("Status"),
    position: 40,
    className: "OMV.module.admin.service.aria2.Status"
});
