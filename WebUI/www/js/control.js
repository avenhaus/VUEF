var control_ui = {
    name: "control_ui",
    elements: [],
    last_index: -1,
    last_sub_index: -1,
    filter: "all",
    error_msg: "" 
};

function controlTab() {
    console.log("ControlTab")
    opentab('controltab', 'mainuitabscontent', 'mainuitablinks'); 
    setup_is_done = true;
    getControlUI();
}

function getControlUI() {
    console.log("getControlUI");
    if (http_communication_locked) {
        document.getElementById('control_status').innerHTML = "Communication locked by another process, retry later.";
        return;
    }
    document.getElementById('control_loader').style.display = "block";
    document.getElementById('control_list_content').style.display = "none";
    document.getElementById('control_status').style.display = "none";
    //document.getElementById('control_refresh_btn').style.display = "none";

    control_ui.elements = [];
    control_ui.last_index =  -1;
    control_ui.last_sub_index = -1;
    control_ui.error_msg = "" ;
    //removeIf(production)

    var response_text = "{\"_INFO_\":{\"L\":\"main\"},\"_VARS_\":[{\"P\":\"LCD_Brightness\",\"L\":\"LCD Brightness\",\"T\":\"uint8\",\"H\":\"Brightness of LCD backlight\",\"V\":200},{\"P\":\"adc_vref\",\"L\":\"adc_vref\",\"T\":\"uint32\",\"H\":\"ADC calibration\",\"V\":0}],\"ROS1\":{\"_INFO_\":{\"L\":\"ROS1\"},\"_VARS_\":[{\"P\":\"Host\",\"L\":\"Host\",\"T\":\"str\",\"H\":\"ROS1 server\",\"V\":\"192.168.0.155\",\"S\":31,\"M\":0},{\"P\":\"Port\",\"L\":\"Port\",\"T\":\"uint16\",\"H\":\"ROS1 server port number\",\"V\":11411}]},\"WIFI\":{\"_INFO_\":{\"L\":\"WIFI\",\"H\":\"Network Control\"},\"_VARS_\":[{\"P\":\"SSID\",\"L\":\"SSID\",\"T\":\"str\",\"H\":\"Name of WIFI network\",\"V\":\"Silent Forest\",\"S\":35,\"M\":0},{\"P\":\"Password\",\"L\":\"Password\",\"T\":\"str\",\"H\":\"Password for WIFI network\",\"F\":8,\"V\":\"Carsten&Naoko\",\"S\":63,\"M\":0},{\"P\":\"Hostname\",\"L\":\"Hostname\",\"T\":\"str\",\"H\":\"Name of this device on the network\",\"V\":\"ROS-remote\",\"S\":31,\"M\":0},{\"P\":\"IP\",\"L\":\"IP\",\"T\":\"IP_ADR\",\"H\":\"Fixed IP address of this device\",\"V\":\"0.0.0.0\"},{\"P\":\"Gateway\",\"L\":\"Gateway\",\"T\":\"IP_ADR\",\"H\":\"Gateway IP address\",\"V\":\"0.0.0.0\"},{\"P\":\"Subnet\",\"L\":\"Subnet\",\"T\":\"IP_ADR\",\"H\":\"Subnet mask\",\"V\":\"0.0.0.0\"},{\"P\":\"DNS\",\"L\":\"DNS\",\"T\":\"IP_ADR\",\"H\":\"Domain Name Server\",\"V\":\"0.0.0.0\"},{\"P\":\"disabled\",\"L\":\"disabled\",\"T\":\"bool\",\"H\":\"Disable networking\",\"V\":0}]}}";
    getESPcontrolSuccess(response_text);
    return;
    //endRemoveIf(production)
    var url = "/command?plain=" + encodeURIComponent("config.control-ui");
    SendGetHttp(url, getESPcontrolSuccess, getESPcontrolfailed)
}

function setESPcontrolSuccess(response) {
    //console.log(response);
}

function setESPcontrolfailed(error_code, response) {
    alertdlg("Set failed", "Error " + error_code + " :" + response);
    console.log("Error " + error_code + " :" + response);
    /*
    document.getElementById('btn_control_' + control_lastindex + "_" + control_lastsubindex).className = "btn btn-danger";
    document.getElementById('icon_control_' + control_lastindex + "_" + control_lastsubindex).className = "form-control-feedback has-error ico_feedback";
    document.getElementById('icon_control_' + control_lastindex + "_" + control_lastsubindex).innerHTML = "<svg width='1.3em' height='1.2em' version='2.0'> <use href='#icon-remove' /> </svg>";
    document.getElementById('status_control_' + control_lastindex + "_" + control_lastsubindex).className = "form-group has-feedback has-error";
    */
}

function getESPcontrolSuccess(response) {
    var docElement = document.getElementById('control_list_data');
    if (!process_settings_answer(docElement, control_ui, response)) {
        getESPcontrolfailed(406, "Wrong data");
        console.log(response);
        return;
    }
    document.getElementById('control_loader').style.display = "none";
    document.getElementById('control_list_content').style.display = "block";
    document.getElementById('control_status').style.display = "none";
    // document.getElementById('control_refresh_btn').style.display = "block";
}

function getESPcontrolfailed(error_code, response) {
    console.log("Error " + error_code + " :" + response);
    document.getElementById('control_loader').style.display = "none";
    document.getElementById('control_status').style.display = "block";
    document.getElementById('control_status').innerHTML = "Failed:" + error_code + " " + response;
    //document.getElementById('control_refresh_btn').style.display = "block";
}

function build_state_from_index(sentry) {
    content = "<div class='item-flex-row'>";
    content += "<span id='state_" + sentry.pname.replace(".", "_") + "' class='bgtrans'>" + sentry.defaultvalue + "</span>";
    content += "</div>";
    return content;
}

function process_state_update(json_text) {
    console.log("process_state_update");
    var result = 0;
    try {
        var update = JSON.parse(json_text);
        for (var key in update) {
            el = document.getElementById("state_" + key.replace(".", "_"));
            if (el) { 
                el.innerHTML = update[key]; 
                el.classList.add("highlight");
                setTimeout(function (elm) {
                    elm.classList.remove("highlight");
                }, 1000, el);
            }
        }
    } catch (e) {
        console.error("State update parsing error:", e);
        result = 0;
    }
    return result;
}