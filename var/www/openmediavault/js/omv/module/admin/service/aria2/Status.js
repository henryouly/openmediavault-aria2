Ext.define("OMV.module.admin.service.aria2.Status", {
	extend: "OMV.workspace.grid.Panel",
	requires: [
		"OMV.Rpc",
		"OMV.data.Store",
		"OMV.data.Model",
		"OMV.data.proxy.Rpc"
	],
	hideAddButton: true,
	hideEditButton: true,
	hideDeleteButton: true,
	hideRefreshButton: false,
	hidePagingToolbar: true,
	disableSelection: true,
	stateful: true,
	stateId: "43c44e0a-ad79-4d9f-bf7b-4fdccabdd709",
	columns: [{
		text: _("File name"),
		sortable: true,
		dataIndex: "filename",
		stateId: "filename",
        flex: 1
	},{
		text: _("Percent"),
		sortable: false,
		dataIndex: "percent",
		stateId: "percent",
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
                        { name: "uuid", type: "string" },
                        { name: "filename", type: "string" },
                        { name: "percent", type: "string" },
                        { name: "eta", type: "string" }
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
    }
});

OMV.WorkspaceManager.registerPanel({
	id: "yaaw",
	path: "/service/aria2",
	text: _("Status"),
	position: 40,
	className: "OMV.module.admin.service.aria2.Status"
});
