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
                "percent" => round(100.0 * $completedLength / $totalLength, 2) . "%",
                "eta" => $this->readableTime($etaInSecond),
                "downloadSpeed" => $this->readableSpeed($downloadSpeed, 2)
            );
        }
        return $tasks;
    }

    private function readableTime($seconds)
    {
        if ($seconds >= 86400) {
            return "> 1 day";
        }
        return gmdate("H:i:s", $seconds);
    }

    private function readableSpeed($speed, $round = 0)
    {
        if ($speed == 0) return "-";
        $sizes = array('Bytes/s', 'KB/s', 'MB/s', 'GB/s');
        for ($i = 0; $speed > 1024 && isset($sizes[$i + 1]); $i++) $speed /= 1024;
        return round($speed, $round) . $sizes[$i];
    }

    public function addTask($url, $paused)
    {
        $result = $this->rpc->addUri(array($url), array('paused' => $paused));
        return $result;
    }

    public function deleteTask($id, $status, $deleteLocalData)
    {
        if ($status === "complete" || $status === "error" || $status === "removed") {
            $result = $this->rpc->removeDownloadResult($id);
        } else {
            $result = $this->rpc->remove($id);
        }
        return $result;
    }

    public function pauseTask($id)
    {
        $result = $this->rpc->pause($id);
        return $result;
    }

    public function resumeTask($id)
    {
        $result = $this->rpc->unpause($id);
        return $result;
    }

    public function deleteLocalFile($id)
    {
        $result = $this->rpc->getFiles($id);
        foreach ($result['result'] as $file) {
            unlink($file['path']);
        }
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
        $this->registerMethod("pauseTask");
        $this->registerMethod("resumeTask");
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
                    "max-concurrent-downloads":{"type":"integer"},
                    "max-overall-download-limit":{"type":"integer"}
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
            "max-concurrent-downloads" => $params["max-concurrent-downloads"],
            "max-overall-download-limit" => $params["max-overall-download-limit"]
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
                "downloadSpeed" => $task['downloadSpeed'],
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
        $status = $params["status"];
        $deleteLocalData = boolval($params["delete_local_data"]);
        if ($deleteLocalData) {
            $server->deleteLocalFile($id);
        }
        return $server->deleteTask($id, $status, $deleteLocalData);
    }

    public function pauseTask($params, $context)
    {
        $this->validateMethodContext($context, array(
            "role" => OMV_ROLE_ADMINISTRATOR
        ));

        $server = $this->aria2RpcServer();
        $id = $params["id"];
        return $server->pauseTask($id);
    }

    public function resumeTask($params, $context)
    {
        $this->validateMethodContext($context, array(
            "role" => OMV_ROLE_ADMINISTRATOR
        ));

        $server = $this->aria2RpcServer();
        $id = $params["id"];
        return $server->resumeTask($id);
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
        $server = "192.168.30.139";

        $url = "http://" . $server . ":" . $object["rpcport"] . $object["rpcurl"];

        $server = new Aria2Server();
        $server->connect($url);

        return $server;
    }
}

// Register the RPC service.
$rpcServiceMgr = &OMVRpcServiceMgr::getInstance();
$rpcServiceMgr->registerService(new OMVRpcServiceAria2());
