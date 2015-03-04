<?php

require_once("openmediavault/config.inc");
require_once("openmediavault/error.inc");
require_once("openmediavault/notify.inc");
require_once("openmediavault/object.inc");
require_once("openmediavault/rpcservice.inc");
require_once("openmediavault/util.inc");

class Aria2Rpc {
    private $server;
    private $ch;
    function __construct($server='http://127.0.0.1:6800/jsonrpc'){
        $this->server = $server;
        $this->ch = curl_init($server);
        curl_setopt_array($this->ch,array(
            CURLOPT_POST=>true,
            CURLOPT_RETURNTRANSFER=>true,
            CURLOPT_HEADER=>false,
        ));
    }
    function __destruct(){
        curl_close($this->ch);
    }
    private function req($data){
        curl_setopt($this->ch,CURLOPT_POSTFIELDS,$data);        
        return curl_exec($this->ch);
    }
    function __call($name,$arg){
        $data = array(
            'jsonrpc'=>'2.0',
            'id'=>'1',
            'method'=>'aria2.'.$name,
            'params'=>$arg,
        );
        $data = json_encode($data);
        return json_decode($this->req($data),1);
    }
}

class Aria2Server
{
    private $rpc;

    public function connect($url)
    {
        $this->rpc = new Aria2Rpc($url);
    }

    public function getTasks()
    {
        $tasks = array();

        $result = $this->rpc->tellActive();
        $tasks = array_merge($tasks, $this->parseTask($result));
        $result = $this->rpc->tellWaiting(0, 1000);
        $tasks = array_merge($tasks, $this->parseTask($result));
        $result = $this->rpc->tellStopped(0, 1000);
        $tasks = array_merge($tasks, $this->parseTask($result));
        return $tasks;
    }

    private function parseTask($result)
    {
        $tasks = array();
        foreach ($result['result'] as $res) {
            $file = $res['files'][0];
            $downloadSpeed = intval($res['downloadSpeed']);
            $completedLength = intval($res['completedLength']);
            $totalLength = intval($res['totalLength']);
            $etaInSecond = ($totalLength - $completedLength) / $downloadSpeed;
            $tasks[] = array(
                "id" => $res['gid'],
                "filename" => basename($file['path']),
                // "filename" => $res['gid'],
                "status" => $res['status'],
                "total" => $totalLength,
                "completed" => $completedLength,
                "percent" => intval(1000 * $completedLength / $totalLength) / 10 . "%",
                "eta" => $this->toETATime($etaInSecond)
            );
        }
        return $tasks;
    }

    private function toETATime($seconds)
    {
        if ($seconds >= 86400) {
            return "> 1 day";
        }
        return gmdate("H:i:s", $seconds);
    }

    public function addTask($url, $paused)
    {
        $result = $this->rpc->addUri(array($url), array('paused' => $paused));
        return $result;
    }

    public function deleteTask($id, $deleteLocalData)
    {
        $result = $this->rpc->remove($id);
        return $result;
    }
}

class OMVRpcServiceAria2 extends OMVRpcServiceAbstract
{
    /**
     * Get the main event message path of the service. This is a helper 
     * function to avoid "magic numbers".
     *
     * @return string
     */
    private function getEventMessagePath()
    {
        return "org.openmediavault.services.aria2";
    }
    
    /**
     * Get the base XPath of the service. This is a helper function to avoid 
     * "magic numbers".
     *
     * @return string
     */
    private function getXpath()
    {
        return "//services/aria2";
    }
    
    /**
     * Get the name of the RPC service.
     *
     * @return string
     */
    public function getName()
    {
        return "Aria2";
    }

    /**
     * Initialize the RPC service. The RPC methods are registered in this
     * function with $this->registerMethod.
     *
     * @return void
     */
    public function initialize()
    {
        $this->registerMethod("getSettings");
        $this->registerMethod("setSettings");
        $this->registerMethod("getTaskList");
        $this->registerMethod("addTask");
        $this->registerMethod("deleteTask");
    }

    public function getSettings($params, $context)
    {
        // $xmlConfig is needed when reading and writing from the configuration.
        global $xmlConfig;
        
        // Validate the RPC caller context.
        //
        // validateMethodContext takes the currentcontext as the first
        // parameter. The second paramter is the valid context and that can be
        // OMV_ROLE_ADMINISTRATOR, OMV_ROLE_USER or OMV_ROLE_EVERYONE.
        // This is used to make sure that the right user accesses the method.
        $this->validateMethodContext($context, array(
            "role" => OMV_ROLE_ADMINISTRATOR
        ));
        
        // Get the configuration object.
        $object = $xmlConfig->get($this->getXpath());
        
        // If no data was found, throw an exception and provide the XPath that
        // failed.
        if (is_null($object)) {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $this->getXpath()
            );
        }
        
        // Modify result data.
        // boolval and intval converts strings and numbers to their boolean
        // and integer value.
        $object["enable"] = boolval($object["enable"]);
        $object["split"] = intval($object["split"]);
        $object["max-connection-per-server"] = intval($object["max-connection-per-server"]);
        $object["max-concurrent-downloads"] = intval($object["max-concurrent-downloads"]);
        // Remove useless fields from the result object.
        
        return $object;
    }

    public function setSettings($params, $context)
    {
        global $xmlConfig;
        
        $this->validateMethodContext($context, array(
            "role" => OMV_ROLE_ADMINISTRATOR
        ));

        // Validate the parameters of the RPC service method.
        //
        // OpenMediavault uses JSON Schema to validate parameters. A more
        // detailed specification is provided here http://json-schema.org/
        $this->validateMethodParams(
            $params,
            '{
                "type": "object",
                "properties": {
                    "enable": { "type": "boolean" },
                    "dir":{'.$GLOBALS['OMV_JSONSCHEMA_UUID_NONE'].'},
                    "split":{ "type": "integer", "minimum": 1 },
                    "max-connection-per-server":{"type":"integer"},
                    "max-concurrent-downloads":{"type":"integer"}
                }
            }'
        );
        // Get the existing configuration object.
        $object = $xmlConfig->get($this->getXpath());
        if(is_null($object)) {
            throw new OMVException(OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
              $xpath);
        }
        // Update the configuration object.
        $object = array_merge($object, array(
            "enable" => array_boolval($params, "enable"),
            "split" => $params["split"],
            "dir" => $params["dir"],
            "max-connection-per-server" => $params["max-connection-per-server"],
            "max-concurrent-downloads" => $params["max-concurrent-downloads"]
        ));
        
        // Update the configuration file. If it fails it throws an exception.
        if (false === $xmlConfig->replace($this->getXpath(), $object)) {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_SET_OBJECT_FAILED,
                $this->getXpath()
            );
        }
        
        // Notify configuration changes.
        //
        // This will notify event listeners such as the service module
        // to perform certain tasks. The most common one is to mark the
        // service as dirty.
        $dispatcher = &OMVNotifyDispatcher::getInstance();
        $dispatcher->notify(
            OMV_NOTIFY_MODIFY,
            $this->getEventMessagePath(),
            $object
        );

        return $object;
    }

    public function getTaskList($params, $context)
    {
        $this->validateMethodContext($context, array(
            "role" => OMV_ROLE_ADMINISTRATOR
        ));

        $server = $this->aria2RpcServer();
        $tasks = array();
        foreach ($server->getTasks() as $task) {
            $tasks[] = array(
                "id" => $task['id'],
                "filename" => $task['filename'],
                "status" => $task['status'],
                "percent" => $task['percent'],
                "eta" => $task['eta'],
            );
        }
        return $this->applyFilter(
            $tasks,
            $params["start"],
            $params["limit"],
            $params["sortfield"],
            $params["sortdir"]
        );
    }

    public function addTask($params, $context)
    {
        $this->validateMethodContext($context, array(
            "role" => OMV_ROLE_ADMINISTRATOR
        ));

        $server = $this->aria2RpcServer();
        $url = $params["url"];
        $paused = !boolval($params["start_download"]);
        return $server->addTask($url, $paused);
    }

    public function deleteTask($params, $context)
    {
        $this->validateMethodContext($context, array(
            "role" => OMV_ROLE_ADMINISTRATOR
        ));

        $server = $this->aria2RpcServer();
        $id = $params["id"];
        $deleteLocalData = boolval($params["delete_local_data"]);
        $server->deleteTask($id);
    }

    private function aria2RpcServer()
    {
        global $xmlConfig;

        $object = $xmlConfig->get($this->getXpath());
        if (is_null($object)) {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $this->getXpath()
            );
        }
        $object["rpcport"] = 6800;
        $object["rpcurl"] = "/jsonrpc";

        $url = "http://192.168.30.139:" . $object["rpcport"] . $object["rpcurl"];

        $server = new Aria2Server();
        $server->connect($url);

        return $server;
    }
}

// Register the RPC service.
$rpcServiceMgr = &OMVRpcServiceMgr::getInstance();
$rpcServiceMgr->registerService(new OMVRpcServiceAria2());
