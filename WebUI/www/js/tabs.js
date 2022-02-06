var currentTab;

function opentab(tabname, tabcontentid, tablinkid) {
    /*
    var i, tabcontent, tablinks;
    tabcontent = document.getElementsByClassName("tabcontent");
    for (i = 0; i < tabcontent.length; i++) {
        if (tabcontent[i].parentNode.id == tabcontentid) {
            tabcontent[i].style.display = "none";
        }
    }
    tablinks = document.getElementsByClassName("tablinks");
    for (i = 0; i < tablinks.length; i++) {
        if (tablinks[i].parentNode.id == tablinkid) {
            tablinks[i].className = tablinks[i].className.replace(" active", "");
        }
    }*/
    if (currentTab) { currentTab.style.display = "none"; } 
    currentTab = document.getElementById(tabname)
    currentTab.style.display = "block";
}