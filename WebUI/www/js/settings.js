
var settings_ui = {
    name: "settings_ui",
    elements: [],
    last_index: -1,
    last_sub_index: -1,
    filter: "all",
    error_msg: "" 
};

// var setting_configList = [];
// var setting_error_msg = "";
// var setting_lastindex = -1;
// var setting_lastsubindex = -1;
// var current_setting_filter = 'network';

var setup_is_done = false;
var EP_STA_SSID = "WIFI.SSID";
var wizard_mode = false;
var RF_PASSWORD = (1<<5);
var RF_WIZARD = (1<<6);

function wizardTab() {
    console.log("Wizard")
    wizard_mode = true;
    document.getElementById('settings_tab_title').innerHTML = "&#129497; Wizard"
    opentab('settingstab', 'mainuitabscontent', 'mainuitablinks'); 
    setup_is_done = true;
    refreshSettings();
}

function settingsTab() {
    console.log("SettingsTab")
    wizard_mode = false;
    document.getElementById('settings_tab_title').innerHTML = "Settings"
    opentab('settingstab', 'mainuitabscontent', 'mainuitablinks'); 
    setup_is_done = true;
    refreshSettings();
}

function saveConfig() {
    if (http_communication_locked) {
        document.getElementById('config_status').innerHTML = "Communication locked by another process, retry later.";
        return;
    }
    var url = "/command?plain=" + encodeURIComponent("config.save");
    SendGetHttp(url, saveESPsettingsSuccess, saveESPsettingsFailed);
}

function saveESPsettingsSuccess(response) {
    document.getElementById('settings_status').style.display = "block";
    document.getElementById('settings_status').innerHTML = "Settings saved.";
}

function saveESPsettingsFailed(error_code, response) {
    console.log("Save Error " + error_code + " :" + response);
    document.getElementById('settings_status').style.display = "block";
    document.getElementById('settings_status').innerHTML = "Save Failed:" + error_code + " " + response;
}

function refreshSettings() {
    console.log("refreshSettings");
    if (http_communication_locked) {
        document.getElementById('config_status').innerHTML = "Communication locked by another process, retry later.";
        return;
    }
    document.getElementById('settings_loader').style.display = "block";
    document.getElementById('settings_list_content').style.display = "none";
    document.getElementById('settings_status').style.display = "none";
    document.getElementById('settings_refresh_btn').style.display = "none";

    settings_ui.elements = [];
    //removeIf(production)

    var response_text = "{\"_INFO_\":{\"L\":\"main\"},\"_VARS_\":[{\"P\":\"LCD_Brightness\",\"L\":\"LCD Brightness\",\"T\":\"uint8\",\"H\":\"Brightness of LCD backlight\",\"V\":200},{\"P\":\"adc_vref\",\"L\":\"adc_vref\",\"T\":\"uint32\",\"H\":\"ADC calibration\",\"V\":0}],\"ROS1\":{\"_INFO_\":{\"L\":\"ROS1\"},\"_VARS_\":[{\"P\":\"Host\",\"L\":\"Host\",\"T\":\"str\",\"H\":\"ROS1 server\",\"V\":\"192.168.0.155\",\"S\":31,\"M\":0},{\"P\":\"Port\",\"L\":\"Port\",\"T\":\"uint16\",\"H\":\"ROS1 server port number\",\"V\":11411}]},\"WIFI\":{\"_INFO_\":{\"L\":\"WIFI\",\"H\":\"Network Settings\"},\"_VARS_\":[{\"P\":\"SSID\",\"L\":\"SSID\",\"T\":\"str\",\"H\":\"Name of WIFI network\",\"V\":\"Silent Forest\",\"S\":35,\"M\":0},{\"P\":\"Password\",\"L\":\"Password\",\"T\":\"str\",\"H\":\"Password for WIFI network\",\"F\":8,\"V\":\"Carsten&Naoko\",\"S\":63,\"M\":0},{\"P\":\"Hostname\",\"L\":\"Hostname\",\"T\":\"str\",\"H\":\"Name of this device on the network\",\"V\":\"ROS-remote\",\"S\":31,\"M\":0},{\"P\":\"IP\",\"L\":\"IP\",\"T\":\"IP_ADR\",\"H\":\"Fixed IP address of this device\",\"V\":\"0.0.0.0\"},{\"P\":\"Gateway\",\"L\":\"Gateway\",\"T\":\"IP_ADR\",\"H\":\"Gateway IP address\",\"V\":\"0.0.0.0\"},{\"P\":\"Subnet\",\"L\":\"Subnet\",\"T\":\"IP_ADR\",\"H\":\"Subnet mask\",\"V\":\"0.0.0.0\"},{\"P\":\"DNS\",\"L\":\"DNS\",\"T\":\"IP_ADR\",\"H\":\"Domain Name Server\",\"V\":\"0.0.0.0\"},{\"P\":\"disabled\",\"L\":\"disabled\",\"T\":\"bool\",\"H\":\"Disable networking\",\"V\":0}]}}";
    getESPsettingsSuccess(response_text);
    return;
    //endRemoveIf(production)
    var url = "/command?plain=" + encodeURIComponent(wizard_mode ? "config.wizard-ui" : "config.ui");
    SendGetHttp(url, getESPsettingsSuccess, getESPsettingsfailed)
}

function build_select_flag_for_setting_list(uicfg, index, subindex) {
    var html = "";
    var flag =
        html += "<select class='form-control' id='setting_" + index + "_" + subindex + "' onchange='setting_checkchange(" + uicfg.name + "," + index + "," + subindex + ")' >";
    html += "<option value='1'";
    var tmp = uicfg.elements[index].defaultvalue
    tmp |= settings_get_flag_value(uicfg, index, subindex);
    if (tmp == uicfg.elements[index].defaultvalue) html += " selected ";
    html += ">";
    html += "Disable";
    html += "</option>\n";
    html += "<option value='0'";
    var tmp = uicfg.elements[index].defaultvalue
    tmp &= ~(settings_get_flag_value(index, subindex));
    if (tmp == uicfg.elements[index].defaultvalue) html += " selected ";
    html += ">";
    html += "Enable";
    html += "</option>\n";
    html += "</select>\n";
    //console.log("default:" + uicfg.elements[index].defaultvalue);
    //console.log(html);
    return html;
}

function build_select_for_setting_list(uicfg, index, subindex) {
    var html = "<select class='form-control input-min wauto' id='setting_" + index + "_" + subindex + "' onchange='setting_checkchange(" + uicfg.name + "," + index + "," + subindex + ")' >";
    for (var i = 0; i < uicfg.elements[index].options.length; i++) {
        html += "<option value='" + uicfg.elements[index].options[i].id + "'";
        if (uicfg.elements[index].options[i].id == uicfg.elements[index].defaultvalue) html += " selected ";
        html += ">";
        html += uicfg.elements[index].options[i].display;
        html += "</option>\n";
    }
    html += "</select>\n";
    //console.log("default:" + uicfg.elements[index].defaultvalue);
    //console.log(html);
    return html;
}

function build_setting_from_index(uicfg, index, extra_set_function) {
    var i = index;
    var content = "<table>";
    if (i < uicfg.elements.length) {
        var nbsub = 1;
        if (uicfg.elements[i].type == "bool--") {
            nbsub = uicfg.elements[i].options.length;
        }
        for (var sub_element = 0; sub_element < nbsub; sub_element++) {
            if (sub_element > 0) {
                content += "<tr><td style='height:10px;'></td></tr>";
            }
            content += "<tr><td style='vertical-align: middle;'>";
            if (uicfg.elements[i].type == "bool--") {
                content += uicfg.elements[i].options[sub_element].display;
                content += "</td><td>&nbsp;</td><td>";
            }

            content += "<div id='status_setting_" + i + "_" + sub_element + "' class='form-group has-feedback' style='margin: auto;'>";
            content += "<div class='item-flex-row'>";
            content += "<table><tr><td>";
            content += "<div class='input-group'>";
            content += "<div class='input-group-btn'>";
            content += "<button class='btn btn-default btn-svg' onclick='setting_revert_to_default(" + i + "," + sub_element + ")' >";
            content += "<svg width='1.3em' height='1.2em' version='2.0'> <use href='#icon-repeat' /> </svg>";
            content += "</button>";
            content += "</div>";
            content += "<input class='hide_it'></input>";
            content += "</div>";
            content += "</td><td>";
            content += "<div class='input-group'>";
            content += "<span class='input-group-addon hide_it' ></span>";
            //flag
            if (uicfg.elements[i].type == "bool--") {
                //console.log(uicfg.elements[i].label + " " + uicfg.elements[i].type);
                //console.log(uicfg.elements[i].options.length);
                content += build_select_flag_for_setting_list(i, sub_element);
            }
            //drop list
            else if (uicfg.elements[i].options.length > 0) {
                content += build_select_for_setting_list(i, sub_element);
            }
            //text
            else {
                content += "<input id='setting_" + i + "_" + sub_element + "' type='text' class='form-control input-min'  value='" + uicfg.elements[i].defaultvalue + "' onkeyup='setting_checkchange(" + uicfg.name + "," + i + "," + sub_element + ")' >";
            }
            content += "<span id='icon_setting_" + i + "_" + sub_element + "'class='form-control-feedback ico_feedback'></span>";
            content += "<span class='input-group-addon hide_it' ></span>";
            content += "</div>";
            content += "</td></tr></table>";
            content += "<div class='input-group'>";
            content += "<input class='hide_it'></input>";
            content += "<div class='input-group-btn'>";
            content += "<button  id='btn_setting_" + i + "_" + sub_element + "' class='btn btn-default' onclick='settingsetvalue(" + uicfg.name + "," + i + "," + sub_element + ");";
            if (typeof extra_set_function != 'undefined') {
                content += extra_set_function + "(" + i + ");"
            }
            content += "' translate english_content='Set' >" + "Set" + "</button>";
            if (uicfg.elements[i].pname == EP_STA_SSID) {
                content += "<button class='btn btn-default btn-svg' onclick='scanwifidlg(\"" + i + "\",\"" + sub_element + "\")'>";
                content += "<svg width='1.3em' height='1.2em' version='2.0'> <use href='#icon-search' /> </svg>";
                content += "</button>";
            }
            content += "</div>";
            content += "</div>";
            content += "</div>";
            content += "</div>";
            content += "</td></tr>";
        }
    }
    content += "</table>";
    return content;
}

function build_HTML_setting_list(docElement, uicfg) {
    //console.log("build_HTML_setting_list: ", uicfg.elements.length)
    //this to prevent concurent process to update after we clean content
    //if (do_not_build_settings) return;
    var content = "";
    //document.getElementById(uicfg.filter + "_setting_filter").checked = true;
    for (var i = 0; i < uicfg.elements.length; i++) {
        var sentry = uicfg.elements[i];
        // console.log(i, sentry.etype)
        if (sentry.etype == "GROUP") {
            content += "<div>";
            if (sentry.level) { 
                content += "<h3";
                if (sentry.help) {
                    content += " class='tooltip'><span class='tooltip-text'>" + sentry.help + " </span";
                }
                content += ">"+ sentry.label + "</h3>"; 
            } else { content += "<br>"}
            content += "<table class=\"table table-bordered table-striped table-hover table-responsive\" style='width:auto;'><tbody>";
        } else if (sentry.etype == "GROUP_END") {
            content += "</tbody></table></div>"
        } else if (sentry.etype == "CFG" && (uicfg.filter == "all" || (typeof sentry.C !== 'undefined' && sentry.C.trim().toLowerCase() == uicfg.filter))) {
            content += "<tr>";
            content += "<td style='vertical-align:middle'"
            if (sentry.help) {
                content += " class='tooltip'><span class='tooltip-text'>" + sentry.help + " </span";
            }
            content += ">" + sentry.label;
            if (!wizard_mode && sentry.flags & RF_WIZARD) { content += "<span style='color:orange;'>&#9733;</span>"; }
            content += "</td>";
            content += "<td style='vertical-align:middle'>";
            content += "<table><tr><td>" + build_setting_from_index(uicfg, i) + "</td></tr></table>";
            content += "</td>";
            content += "</tr>\n";
        } else if (sentry.etype == "STATE" && (uicfg.filter == "all" || (typeof sentry.C !== 'undefined' && sentry.C.trim().toLowerCase() == uicfg.filter))) {
            content += "<tr>";
            content += "<td style='vertical-align:middle'"
            if (sentry.help) {
                content += " class='tooltip'><span class='tooltip-text'>" + sentry.help + " </span";
            }
            content += ">" + sentry.label;
            content += "</td>";
            content += "<td style='vertical-align:middle'>";
            content += "<table><tr><td>" + build_state_from_index(sentry) + "</td></tr></table>";
            content += "</td>";
            content += "</tr>\n";
        } else {
            console.log("Unknown UI entry type:", sentry.etype);
        }

    }
    if (content.length > 0) docElement.innerHTML = content;
}

function setting_check_value(uicfg, value, index, subindex) {
    var valid = true;
    var entry = uicfg.elements[index];
    //console.log("checking value");
    if (entry.type == "bool--") return valid;
    //does it part of a list?
    if (entry.options.length > 0) {
        var in_list = false;
        for (var i = 0; i < entry.options.length; i++) {
            //console.log("checking *" + entry.options[i].id + "* and *"+ value + "*" );
            if (entry.options[i].id == value) in_list = true;
        }
        valid = in_list;
        if (!valid) setting_error_msg = " in provided list";
    }
    //check byte / integer
    if (entry.type == "B" || entry.type == "I") {
        //cannot be empty
        value.trim();
        if (value.length == 0) valid = false;
        //check minimum?
        if (parseInt(entry.min_val) > parseInt(value)) valid = false;
        //check maximum?
        if (parseInt(entry.max_val) < parseInt(value)) valid = false;
        if (!valid) setting_error_msg = " between " + entry.min_val + " and " + entry.max_val;
        if (isNaN(value)) valid = false;
    } else if (entry.type == "str") {
        //check minimum?
        if (entry.min_val > value.length) valid = false;
        //check maximum?
        if (entry.max_val < value.length) valid = false;
        if (value == "********") valid = false;
        if (!valid) setting_error_msg = " between " + entry.min_val + " char(s) and " + entry.max_val + " char(s) long, and not '********'";
    } else if (entry.type == "IP_ADR") {
        //check ip address
        var ipformat = /^(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$/;
        if (!value.match(ipformat)) {
            valid = false;
            setting_error_msg = " a valid IP format (xxx.xxx.xxx.xxx)";
        }
    }
    return valid;
}


function process_settings_group(uicfg, name, level, gpath, group) {
    var result = 0;
    if (name == "_INFO_") {
        create_setting_group_start(uicfg, name, gpath, level, group);
    } else if (name == "_VARS_") {
        if (group.length > 0) {
            for (var i = 0; i < group.length; i++) {
                create_setting_entry(uicfg, gpath, group[i]);
                result++;
            }
        }
        create_setting_group_end(uicfg, name, level, gpath, group);
    } else {
        for (var key in group) {
            var tp = gpath ?  gpath + '.' + name : name;
            result += process_settings_group(uicfg, key, level+1, tp, group[key]);
        }    
    }
    return result;
}

function process_settings_answer(docElement, uicfg, response_text) {
    // console.log("process_settings_answer");
    var result = 0;
    try {
        var response = JSON.parse(response_text);
        for (var key in response) {
            result += process_settings_group(uicfg, key, 0, "", response[key]);
        }
        if (result > 0) {
            if (setup_is_done) build_HTML_setting_list(docElement, uicfg);           
            //build_HTML_setting_list(docElement, uicfg);           
        }
    } catch (e) {
        console.error("Parsing error:", e);
        result = 0;
    }
    return result;
}

function create_setting_group_start(uicfg, name, level, gpath, group) {
    var config_entry = {
        index: uicfg.elements.length,
        etype: "GROUP",
        level: level,
        name: name,
        label: group.L,
        help: group.H,
        path: gpath
    };
    uicfg.elements.push(config_entry);
}

function create_setting_group_end(uicfg, name, level, gpath, group) {
    var config_entry = {
        index: uicfg.elements.length,
        etype: "GROUP_END",
        level: level,
        name: name,
        label: group.L,
        path: gpath
    };
    uicfg.elements.push(config_entry);    
}


function create_setting_entry(uicfg, gpath, sentry) {
    if (!is_setting_entry(sentry)) return;
    var pname = gpath ? gpath + '.' + sentry.P : sentry.P;
    var slabel = sentry.L;
    var shelp = sentry.H;
    var svalue = sentry.V;
    var scmd = sentry.E ? "" : "config.var " + pname + " ";
    var options = [];
    var min;
    var max;
    if (typeof sentry.M !== 'undefined') {
        min = sentry.M;
    } else { //add limit according the type
        if (sentry.T == "B") min = -127;
        else if (sentry.T == "str") min = 0;
        else if (sentry.T == "IP_ADDR") min = 7;
        else if (sentry.T == "int32") min = -2147483648;
        else if (sentry.T == "int16") min = -32768;
        else if (sentry.T == "int8") min = -128;
        else if (sentry.T == "uint32") max = 0;
        else if (sentry.T == "uint16") max = 0;
        else if (sentry.T == "uint8") max = 0;
        //else if (sentry.T == "float") min = -Infinity;
    }
    if (typeof sentry.S !== 'undefined') {
        max = sentry.S;
    } else { //add limit according the type
        if (sentry.T == "B") max = 255;
        else if (sentry.T == "str") max = 255;
        else if (sentry.T == "IP_ADR") max = 15;
        else if (sentry.T == "int32") max = 2147483647;
        else if (sentry.T == "int16") max = 32767;
        else if (sentry.T == "int8") max = 127;
        else if (sentry.T == "uint32") max = 4294967295;
        else if (sentry.T == "uint16") max = 65535;
        else if (sentry.T == "uint8") max = 255;
        //else if (sentry.T == "float") max = Infinity;
    }
    //list possible options if defined
    if (typeof sentry.O !== 'undefined') {
        console.log("options: " + sentry.O);
        for (var i in sentry.O) {
            var key = i;
            var val = sentry.O[i];
            for (var j in val) {
                var sub_key = j;
                var sub_val = val[j];
                sub_val = sub_val.trim();
                sub_key = sub_key.trim();
                var option = {
                    id: sub_val,
                    display: sub_key
                };
                options.push(option);
                //console.log("*" + sub_key + "* and *" + sub_val + "*");
            }
        }
    }
    //svalue = svalue.trim();
    //create entry in list

    var config_entry = {
        index: uicfg.elements.length,
        etype: sentry.F & 1 ? "CFG" : "STATE",
        label: slabel,
        help: shelp,
        defaultvalue: svalue,
        cmd: scmd,
        options: options,
        min_val: min,
        max_val: max,
        type: sentry.T,
        pname: pname,
        flags: sentry.F
    };
    uicfg.elements.push(config_entry);
    return;
}
//check it is valid entry
function is_setting_entry(sline) {
    if (typeof sline.T === 'undefined' || typeof sline.V === 'undefined' || typeof sline.P === 'undefined' || typeof sline.H === 'undefined') {
        return false
    }
    return true;
}

function settings_get_flag_value(uicfg, index, subindex) {
    var flag = 0;
    if (uicfg.elements[index].type != "bool--") return -1;
    if (uicfg.elements[index].options.length <= subindex) return -1;
    flag = parseInt(uicfg.elements[index].options[subindex].id);
    return flag;
}

function settings_get_flag_description(uicfg, index, subindex) {
    if (uicfg.elements[index].type != "bool--") return -1;
    if (uicfg.elements[index].options.length <= subindex) return -1;
    return uicfg.elements[index].options[subindex].display;
}

function setting_revert_to_default(uicfg, index, subindex) {
    var sub = 0;
    if (typeof subindex != 'undefined') sub = subindex;
    if (uicfg.elements[index].type == "bool--") {
        var flag = settings_get_flag_value(index, subindex);
        var enabled = 0;
        var tmp = parseInt(uicfg.elements[index].defaultvalue);
        tmp |= flag;
        if (tmp == parseInt(uicfg.elements[index].defaultvalue)) document.getElementById('setting_' + index + "_" + sub).value = "1";
        else document.getElementById('setting_' + index + "_" + sub).value = "0";
    } else document.getElementById('setting_' + index + "_" + sub).value = uicfg.elements[index].defaultvalue
    document.getElementById('btn_setting_' + index + "_" + sub).className = "btn btn-default";
    document.getElementById('status_setting_' + index + "_" + sub).className = "form-group has-feedback";
    document.getElementById('icon_setting_' + index + "_" + sub).innerHTML = "";
}

function settingsetvalue(uicfg, index, subindex) {
    var sub = 0;
    if (typeof subindex != 'undefined') sub = subindex;
    //remove possible spaces
    value = document.getElementById('setting_' + index + "_" + sub).value.trim();
    //Apply flag here
    if (uicfg.elements[index].type == "bool--") {
        var tmp = uicfg.elements[index].defaultvalue;
        if (value == "1") {
            tmp |= settings_get_flag_value(index, subindex);
        } else {
            tmp &= ~(settings_get_flag_value(index, subindex));
        }
        value = tmp;
    }
    if (value == uicfg.elements[index].defaultvalue) return;
    //check validity of value
    var isvalid = setting_check_value(uicfg, value, index, subindex);
    //if not valid show error
    if (!isvalid) {
        setsettingerror(index);
        alertdlg("Out of range", "Value must be " + setting_error_msg + " !");
    } else {
        //value is ok save it
        var cmd = uicfg.elements[index].cmd + value;
        uicfg.last_index = index;
        uicfg.last_sub_index = subindex;
        uicfg.elements[index].defaultvalue = value;
        document.getElementById('btn_setting_' + index + "_" + sub).className = "btn btn-success";
        document.getElementById('icon_setting_' + index + "_" + sub).className = "form-control-feedback has-success ico_feedback";
        document.getElementById('icon_setting_' + index + "_" + sub).innerHTML = "<svg width='1.3em' height='1.2em' version='2.0'> <use href='#icon-ok' /> </svg>";
        document.getElementById('status_setting_' + index + "_" + sub).className = "form-group has-feedback has-success";
        var url = "/command?plain=" + encodeURIComponent(cmd);
        SendGetHttp(url, setESPsettingsSuccess, setESPsettingsfailed);
    }
}

function setting_checkchange(uicfg, index, subindex) {
    // console.log("list value changed: ", uicfg );
    var val = document.getElementById('setting_' + index + "_" + subindex).value.trim();
    if (uicfg.elements[index].type == "bool--") {
        //console.log("it is flag value");
        var tmp = uicfg.elements[index].defaultvalue;
        if (val == "1") {
            tmp |= settings_get_flag_value(index, subindex);
        } else {
            tmp &= ~(settings_get_flag_value(index, subindex));
        }
        val = tmp;
    }
    //console.log("value: " + val);
    //console.log("default value: " + uicfg.elements[index].defaultvalue);
    if (uicfg.elements[index].defaultvalue == val) {
        console.log("values are identical");
        document.getElementById('btn_setting_' + index + "_" + subindex).className = "btn btn-default";
        document.getElementById('icon_setting_' + index + "_" + subindex).className = "form-control-feedback";
        document.getElementById('icon_setting_' + index + "_" + subindex).innerHTML = "";
        document.getElementById('status_setting_' + index + "_" + subindex).className = "form-group has-feedback";
    } else if (setting_check_value(uicfg, val, index, subindex)) {
        //console.log("Check passed");
        setsettingchanged(index, subindex);
    } else {
        console.log("change bad");
        setsettingerror(index, subindex);
    }

}

function setsettingchanged(index, subindex) {
    document.getElementById('status_setting_' + index + "_" + subindex).className = "form-group has-feedback has-warning";
    document.getElementById('btn_setting_' + index + "_" + subindex).className = "btn btn-warning";
    document.getElementById('icon_setting_' + index + "_" + subindex).className = "form-control-feedback has-warning ico_feedback";
    document.getElementById('icon_setting_' + index + "_" + subindex).innerHTML = "<svg width='1.3em' height='1.2em' version='2.0'> <use href='#icon-warning-sign' /> </svg>";
}

function setsettingerror(index, subindex) {
    document.getElementById('btn_setting_' + index + "_" + subindex).className = "btn btn-danger";
    document.getElementById('icon_setting_' + index + "_" + subindex).className = "form-control-feedback has-error ico_feedback";
    document.getElementById('icon_setting_' + index + "_" + subindex).innerHTML = "<svg width='1.3em' height='1.2em' version='2.0'> <use href='#icon-remove' /> </svg>";
    document.getElementById('status_setting_' + index + "_" + subindex).className = "form-group has-feedback has-error";
}

function setESPsettingsSuccess(response) {
    //console.log(response);
}

function setESPsettingsfailed(uicfg, error_code, response) {
    alertdlg("Set failed", "Error " + error_code + " :" + response);
    console.log("Error " + error_code + " :" + response);
    document.getElementById('btn_setting_' + uicfg.last_index + "_" + uicfg.last_sub_index).className = "btn btn-danger";
    document.getElementById('icon_setting_' + uicfg.last_index + "_" + uicfg.last_sub_index).className = "form-control-feedback has-error ico_feedback";
    document.getElementById('icon_setting_' + uicfg.last_index + "_" + uicfg.last_sub_index).innerHTML = "<svg width='1.3em' height='1.2em' version='2.0'> <use href='#icon-remove' /> </svg>";
    document.getElementById('status_setting_' + uicfg.last_index + "_" + uicfg.last_sub_index).className = "form-group has-feedback has-error";
}

function getESPsettingsSuccess(response) {
    var docElement = document.getElementById('settings_list_data');
    if (!process_settings_answer(docElement, settings_ui, response)) {
        getESPsettingsfailed(406, "Wrong data");
        console.log(response);
        return;
    }
    document.getElementById('settings_loader').style.display = "none";
    document.getElementById('settings_list_content').style.display = "block";
    document.getElementById('settings_status').style.display = "none";
    document.getElementById('settings_refresh_btn').style.display = "block";
}

function getESPsettingsfailed(error_code, response) {
    console.log("Error " + error_code + " :" + response);
    document.getElementById('settings_loader').style.display = "none";
    document.getElementById('settings_status').style.display = "block";
    document.getElementById('settings_status').innerHTML = "Failed:" + error_code + " " + response;
    document.getElementById('settings_refresh_btn').style.display = "block";
}

function restart_esp() {
    confirmdlg("Please Confirm", "Restart ESP32", process_restart_esp);
}


function process_restart_esp(answer) {
    if (answer == "yes") {
        restartdlg();
    }
}
