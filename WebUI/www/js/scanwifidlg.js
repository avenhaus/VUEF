var ssid_item_scanwifi = -1;
var ssid_subitem_scanwifi = -1;
//scanwifi dialog
function scanwifidlg(item, subitem) {
    var modal = setactiveModal('scanwifidlg.html', scanwifidlg_close);
    if (modal == null) return;
    ssid_item_scanwifi = item;
    ssid_subitem_scanwifi = subitem;
    showModal();
    refresh_scanwifi();
}

function refresh_scanwifi() {
    document.getElementById('AP_scan_loader').style.display = 'block';
    document.getElementById('AP_scan_list').style.display = 'none';
    document.getElementById('AP_scan_status').style.display = 'block';
    document.getElementById('AP_scan_status').innerHTML = "Scanning";
    document.getElementById('refresh_scanwifi_btn').style.display = 'none';
    //removeIf(production)
    var response_text = "[{\"SSID\":\"salon\",\"SIG\":60,\"ENC\":3},{\"SSID\":\"ATTJsku9kA\",\"SIG\":50,\"ENC\":3},{\"SSID\":\"\",\"SIG\":50,\"ENC\":3},{\"SSID\":\"salon\",\"SIG\":46,\"ENC\":3},{\"SSID\":\"maviyesil\",\"SIG\":46,\"ENC\":3},{\"SSID\":\"\",\"SIG\":46,\"ENC\":3}]";
    getscanWifiSuccess(response_text);
    return;
    //endRemoveIf(production)
    var url = "/command?plain=" + encodeURIComponent("networks");
    SendGetHttp(url, getscanWifiSuccess, getscanWififailed);
}

function process_scanWifi_answer(response_text) {
    var result = true;
    var content = "";
    try {
        var response = JSON.parse(response_text);
        {
            var aplist = response;
            console.log("found " + aplist.length + " WIFI networks");
            aplist.sort(function(a, b) {
                return (parseInt(a.SIGNAL) < parseInt(b.SIGNAL)) ? -1 : (parseInt(a.SIGNAL) > parseInt(b.SIGNAL)) ? 1 : 0
            });
            for (var i = 0; i < aplist.length; i++) {
                content += "<tr>";
                content += "<td style='vertical-align:middle'>";
                content += aplist[i].SSID;
                content += "</td>";
                content += "<td style='text-align: center;vertical-align:middle;'>";
                content += aplist[i].SIG;
                content += "%</td>";
                content += "<td style='vertical-align:middle'><center>";
                if (aplist[i].ENC > 0) { content += "<svg width='1.3em' height='1.2em' version=\"2.0\"> <use href=\"#icon-lock\" /> </svg>"; }
                content += "</></td>";
                content += "<td>";
                content += "<button class='btn btn-primary' onclick='select_ap_ssid(\"" + aplist[i].SSID.replace("'","\\'").replace("\"","\\\"") + "\");'>";
                content += "<svg width='1.3em' height='1.2em' version=\"2.0\"> <use href=\"#icon-ok\" /> </svg>";
                content += "</button>";
                content += "</td>";
                content += "</tr>";
            }
        }
    } catch (e) {
        console.error("Parsing error:", e);
        result = false;
    }
    document.getElementById('AP_scan_data').innerHTML = content;
    return result;
}

function select_ap_ssid(ssid_name) {
    var val = document.getElementById("setting_" + ssid_item_scanwifi + "_" + ssid_subitem_scanwifi).value;
    document.getElementById("setting_" + ssid_item_scanwifi + "_" + ssid_subitem_scanwifi).value = ssid_name;
    document.getElementById("setting_" + ssid_item_scanwifi + "_" + ssid_subitem_scanwifi).focus();
    if (val != ssid_name)setsettingchanged(ssid_item_scanwifi, ssid_subitem_scanwifi);
    closeModal("Ok");
}

function getscanWifiSuccess(response) {
    if (!process_scanWifi_answer(response)) {
        getscanWififailed(406, "Wrong data");
        return;
    }
    document.getElementById('AP_scan_loader').style.display = "none";
    document.getElementById('AP_scan_list').style.display = "block";
    document.getElementById('AP_scan_status').style.display = "none";
    document.getElementById('refresh_scanwifi_btn').style.display = "block";
}

function getscanWififailed(error_code, response) {
    console.log("Error " + error_code + " :" + response);
    document.getElementById('AP_scan_loader').style.display = "none";
    document.getElementById('AP_scan_status').style.display = "block";
    document.getElementById('AP_scan_status').innerHTML = "Failed:" + error_code + " " + response;
    document.getElementById('refresh_scanwifi_btn').style.display = "block";
}

function scanwifidlg_close(response) {
    //console.log(response);
}
