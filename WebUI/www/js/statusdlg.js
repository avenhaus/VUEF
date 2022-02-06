var statuspage = 0;
var statuscontent = "";
//status dialog
function statusdlg() {
    var modal = setactiveModal('statusdlg.html');
    if (modal == null) return;
    showModal();
    refreshstatus();
}


function statussuccess(response) {
    document.getElementById('refreshstatusbtn').style.display = 'block';
    document.getElementById('status_loader').style.display = 'none';
    var modal = getactiveModal();
    if (modal == null) return;
    var text = modal.element.getElementsByClassName("modal-text")[0];
    var tresponse = response.split("\n");
    statuscontent = "";
    for (var i = 0; i < tresponse.length; i++) {
        var data = tresponse[i].split(":");
        if (data.length >= 2) {
            statuscontent += "<label>" + data[0] + ": </label>&nbsp;<span class='text-info'><strong>";
            var data2 = data[1].split(" (")
            statuscontent += data2[0].trim();
            for (v = 1; v < data2.length; v++) {
                statuscontent += " (" + data2[v];
            }
            for (v = 2; v < data.length; v++) {
                statuscontent += ":" + data[v];
            }
            statuscontent += "</strong></span><br>";
        } //else statuscontent += tresponse[i] + "<br>";
    }
    statuscontent += "</strong></span><br>";
    text.innerHTML = statuscontent;
    //console.log(response);
}

function statusfailed(errorcode, response) {
    document.getElementById('refreshstatusbtn').style.display = 'block';
    document.getElementById('status_loader').style.display = 'none';
    document.getElementById('status_msg').style.display = 'block';
    console.log("Error " + errorcode + " : " + response);
    document.getElementById('status_msg').innerHTML = "Error " + errorcode + " : " + response;
}

function refreshstatus() {
    document.getElementById('refreshstatusbtn').style.display = 'none';
    document.getElementById('status_loader').style.display = 'block';
    var modal = getactiveModal();
    if (modal == null) return;
    var text = modal.element.getElementsByClassName("modal-text")[0];
    text.innerHTML = "";
    document.getElementById('status_msg').style.display = 'none';
    var url = "/command?plain=" + encodeURIComponent("stats");;
    SendGetHttp(url, statussuccess, statusfailed)
}
