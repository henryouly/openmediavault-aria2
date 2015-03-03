<?php

require_once "openmediavault/config.inc";
require_once "openmediavault/error.inc";
require_once "openmediavault/initscript.inc";
require_once "openmediavault/module.inc";
require_once "openmediavault/util.inc";

class OMVModuleAria2 extends OMVModuleServiceAbstract implements
	OMVINotifyListener,
	OMVIModuleServiceStatus
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
		return "/config/services/aria2";
	}

	/**
	 * Get the module name.
	 *
	 * @return string
	 */
	public function getName()
	{
		return "aria2";
	}

	/**
	 * Get the module status.
	 */
	public function getStatus()
	{
		global $xmlConfig;

		// Get the configuration object.
		$object = $xmlConfig->get($this->getXPath());

		if (is_null($object)) {
			throw new OMVException(
				OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
				$this->getXPath()
			);
		}
		
		// Return the status of the service. This information is displayed
		// under Diagnostics/Services.
		return array(
			"name" => $this->getName(),
			"title" => gettext("Aria2"),
			"enabled" => boolval($object["enable"]),
			"running" => false
		);
	}

	/**
	 * Generate the configuration. 
	 */
	public function applyConfig()
	{
		global $xmlConfig;

		$cmd = sprintf("omv-mkconf %s 2>&1", $this->getName());

		if (0 !== $this->exec($cmd, $output)) {
			throw new OMVException(
				OMVErrorMsg::E_EXEC_FAILED,
				$cmd,
				implode("\n", $output)
			);
		}
	}

	/**
	 * Bind listeners.
	 */
	public function bindListeners(OMVNotifyDispatcher $dispatcher)
	{
		$moduleMgr = &OMVModuleMgr::getInstance();
		
		// Add listeners here. The most common thing is to monitor configuration
		// changes on the service. When the config is changed the module
		// sets itself as dirty (as seen below). Setting a module as dirty
		// makes the apply button appear in the web interface (which in turn
		// calls the applyConfig function on each module with a dirty state).
		$dispatcher->addListener(
			OMV_NOTIFY_MODIFY,
			$this->getEventMessagePath(),
			array($this, "setDirty")
		);
	}
}

// Register module
$moduleMgr = &OMVModuleMgr::getInstance();
$moduleMgr->registerModule(new OMVModuleAria2());
