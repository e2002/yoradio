var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
var currentItem = 0;

window.addEventListener('load', onLoad);
function initWebSocket() {
  console.log('Trying to open a WebSocket connection...');
  websocket = new WebSocket(gateway);
  websocket.onopen    = onOpen;
  websocket.onclose   = onClose;
  websocket.onmessage = onMessage;
}
function onOpen(event) {
  console.log('Connection opened');
}
function onClose(event) {
  console.log('Connection closed');
  document.getElementById('playbutton').setAttribute("class", "stopped");
  setTimeout(initWebSocket, 2000);
}

function onMessage(event) {
  var data = JSON.parse(event.data);
  if(data.nameset) document.getElementById('nameset').innerHTML = data.nameset;
  if(data.meta) document.getElementById('meta').innerHTML = data.meta;
  if(data.vol) {
    setVolRangeValue(document.getElementById('volrange'),data.vol);
  }
  if(data.current) setCurrentItem(data.current);
  if(data.file)  generatePlaylist(data.file);
  if(data.bitrate) document.getElementById('bitinfo').innerText = 'bitrate: '+data.bitrate+'kBits';
  if(data.rssi) document.getElementById('rsiinfo').innerText = 'rssi: '+data.rssi+'dBm';
  if(data.mode) {
    document.getElementById('playbutton').setAttribute("class",data.mode);
  }
  if(data.bass) {
  	setVolRangeValue(document.getElementById('eqbass'),data.bass);
  	setVolRangeValue(document.getElementById('eqmiddle'),data.middle);
  	setVolRangeValue(document.getElementById('eqtreble'),data.trebble);
  }
  if(data.balance) {
    setVolRangeValue(document.getElementById('eqbal'),data.balance);
  }
}
function scrollToCurrent(){
  var pl = document.getElementById('playlist');
  var lis = pl.getElementsByTagName('li');
  var plh = pl.offsetHeight;
  var plt = pl.offsetTop;
  var topPos = 0;
  var lih = 0;
  for (var i = 0; i <= lis.length - 1; i++) {
    if(i+1==currentItem) {
      topPos = lis[i].offsetTop;
      lih = lis[i].offsetHeight;
    }
  }
  pl.scrollTo({
    top: topPos-plt-plh/2+lih/2,
    left: 0,
    behavior: 'smooth'
  });
}
function setCurrentItem(item){
  currentItem=item;
  var pl = document.getElementById('playlist');
  var lis = pl.getElementsByTagName('li');
  for (var i = 0; i <= lis.length - 1; i++) {
    lis[i].removeAttribute('class');
    if(i+1==currentItem) {
      lis[i].setAttribute("class","active");

    }
  }
  scrollToCurrent();
}
function generatePlaylist(lines){
  var ul = document.getElementById('playlist');
  ul.innerHTML="";
    lines.forEach((line, index) => {
      li = document.createElement('li');
      li.setAttribute('onclick','playStation(this);');
      li.setAttribute('attr-id', index+1);
      li.setAttribute('attr-name', line.name);
      li.setAttribute('attr-url', line.url);
      li.setAttribute('attr-ovol', line.ovol);
      if(index+1==currentItem){
        li.setAttribute("class","active");
      }
      var span = document.createElement('span');
      span.innerHTML = index+1;
      li.appendChild(document.createTextNode(line.name));
      li.appendChild(span);
      ul.appendChild(li);
      initPLEditor();
    });
    scrollToCurrent();
}
function initPLEditor(){
  ple= document.getElementById('pleditorcontent');
  ple.innerHTML="";
  pllines = document.getElementById('playlist').getElementsByTagName('li');
  for (let i = 0; i <= pllines.length - 1; i++) {
    plitem = document.createElement('li');
    plitem.setAttribute('class', 'pleitem');
    plitem.setAttribute('id', 'plitem'+i);
    let pName = pllines[i].getAttribute('attr-name');
    let pUrl = pllines[i].getAttribute('attr-url');
    let pOvol = pllines[i].getAttribute('attr-ovol');
    plitem.innerHTML = '<span class="grabbable" draggable="true">'+("00"+(i+1)).slice(-3)+'</span>\
          <span class="pleinput plecheck"><input type="checkbox" /></span>\
          <input class="pleinput plename" type="text" value="'+pName+'" maxlength="140" />\
          <input class="pleinput pleurl" type="text" value="'+pUrl+'" maxlength="140" />\
          <input class="pleinput pleovol" type="number" min="-30" max="30" step="1" value="'+pOvol+'" />';
    ple.appendChild(plitem);
  }
}
function playStation(el){
  let lis = document.getElementById('playlist').getElementsByTagName('li');

  for (let i = 0; i <= lis.length - 1; i++) {
    lis[i].removeAttribute('class');
  }
  el.setAttribute("class","active");
  id=el.getAttribute('attr-id');
	xhr = new XMLHttpRequest();
	xhr.open("POST","/",true);
	xhr.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
	xhr.send("playstation="+id);
}
function setVolRangeValue(el, val=null){
	let slave = el.getAttribute('data-slaveid');
  if(val){
    el.value = val;
    document.getElementById(slave).innerText=val;
  }
  document.getElementById(slave).innerText=el.value;
  var value = (el.value-el.min)/(el.max-el.min)*100;
  el.style.background = 'linear-gradient(to right, #bfa73e 0%, #bfa73e ' + value + '%, #272727 ' + value + '%, #272727 100%)';
}

function onRangeVolChange(value) {
	xhr = new XMLHttpRequest();
	xhr.open("POST","/",true);
	xhr.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
	xhr.send("vol=" + value+"&");
}
function onRangeEqChange(el){
	let trebble = document.getElementById('eqtreble').value;
	let middle = document.getElementById('eqmiddle').value;
	let bass = document.getElementById('eqbass').value;
	xhr = new XMLHttpRequest();
	xhr.open("POST","/",true);
	xhr.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
	xhr.send("trebble=" + trebble + "&middle=" + middle + "&bass=" + bass + "&");
}
function onRangeBalChange(el){
	xhr = new XMLHttpRequest();
	xhr.open("POST","/",true);
	xhr.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
	xhr.send("ballance=" + el.value+"&");
}
function showSettings(){
  document.getElementById('pleditorwrap').hidden=true;
  document.getElementById('settings').hidden=false;
}
function showEditor(){
  document.getElementById('settings').hidden=true;
  initPLEditor();
  document.getElementById('pleditorwrap').hidden=false;
}
function doCancel() {
  document.getElementById('settings').hidden=true;
  document.getElementById('pleditorwrap').hidden=true;
}
function doExport() {
  window.open("/data/playlist.csv");
}
function doUpload(finput) {
  var formData = new FormData();
	formData.append("plfile", finput.files[0]);
	var xhr = new XMLHttpRequest();
	xhr.open("POST","/upload",true);
	xhr.send(formData);
}
function doAdd(){
	let ple=document.getElementById('pleditorcontent');
	let plitem = document.createElement('li');
	let cnt=ple.getElementsByTagName('li');
  plitem.setAttribute('class', 'pleitem');
  plitem.setAttribute('id', 'plitem'+(cnt.length));
  plitem.innerHTML = '<span class="grabbable" draggable="true">'+("00"+(cnt.length+1)).slice(-3)+'</span>\
      <span class="pleinput plecheck"><input type="checkbox" /></span>\
      <input class="pleinput plename" type="text" value="" maxlength="140" />\
      <input class="pleinput pleurl" type="text" value="" maxlength="140" />\
      <input class="pleinput pleovol" type="number" min="-30" max="30" step="1" value="0" />';
  ple.appendChild(plitem);
  ple.scrollTo({
		top: ple.scrollHeight,
		left: 0,
		behavior: 'smooth'
  });
}
function doRemove(){
	let items=document.getElementById('pleditorcontent').getElementsByTagName('li');
	let pass=[];
	for (let i = 0; i <= items.length - 1; i++) {
		if(items[i].getElementsByTagName('span')[1].getElementsByTagName('input')[0].checked) {
			pass.push(items[i]);
		}
	}
	if(pass.length==0) {
		alert('Choose something first');
		return;
	}
	for (var i = 0; i < pass.length; i++)
	{
		pass[i].remove();
	}
	items=document.getElementById('pleditorcontent').getElementsByTagName('li');
	for (let i = 0; i <= items.length-1; i++) {
		items[i].getElementsByTagName('span')[0].innerText=("00"+(i+1)).slice(-3);
	}
}
function showEqualizer(isshowing){
	document.getElementById('equalizerbg').classList.toggle('hidden');
}
function submitWiFi(){
  var items=document.getElementById("credentialwrap").getElementsByTagName("li");
  var output="";
  for (var i = 0; i <= items.length - 1; i++) {
    inputs=items[i].getElementsByTagName("input");
    if(inputs[0].value == "") continue;
    output+=inputs[0].value+"\t"+inputs[1].value+"\n";
  }
  if(output!=""){ // Well, let's say, quack.
    xhr = new XMLHttpRequest();
	  xhr.open("POST","/",true);
	  xhr.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
	  xhr.send("wifisettings="+output);
	  document.getElementById("settings").innerHTML="<h2>Settings saved. Rebooting...</h2>";
	  setTimeout(function(){ window.location.reload(); }, 10000);
  }
}
function submitPlaylist(){
  var items=document.getElementById("pleditorcontent").getElementsByTagName("li");
  var output="";
  for (var i = 0; i <= items.length - 1; i++) {
    inputs=items[i].getElementsByTagName("input");
    if(inputs[1].value == "" || inputs[2].value == "") continue;
    let ovol = inputs[3].value;
    if(ovol < -30) ovol = -30;
    if(ovol > 30) ovol = 30;
    output+=inputs[1].value+"\t"+inputs[2].value+"\t"+inputs[3].value+"\n";
  }
  xhr = new XMLHttpRequest();
  xhr.open("POST","/",true);
  xhr.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
  xhr.send("playlist="+output.slice(0, -1));
  document.getElementById('pleditorwrap').hidden=true;
}
function initSliers(){
	var sliders = document.getElementsByClassName('slider');
	for (var i = 0; i <= sliders.length - 1; i++) {
		sliders[i].oninput = function() {
		  setVolRangeValue(this);
		};
		setVolRangeValue(sliders[i], 0);
	}
	return;
	var volslider = document.getElementById("volrange");
	var eqvolslider = document.getElementById("eqvol");
	var balslider = document.getElementById("eqbal");
	volslider.oninput = function() {
    setVolRangeValue(this);
  };
  eqvolslider.oninput = function() {
    setVolRangeValue(this);
  };
  balslider.oninput = function() {
    setVolRangeValue(this);
  };
  setVolRangeValue(volslider, 0);
  setVolRangeValue(eqvolslider, 0);
  setVolRangeValue(balslider, 0);
}
function onLoad(event) {
  initWebSocket();
  initButton();
  initSliers();
  document.getElementById("volrange").addEventListener("wheel", function(e){
    if (e.deltaY < 0){
      this.valueAsNumber += 1;
    }else{
      this.value -= 1;
    }
    websocket.send('volume='+this.value);
    e.preventDefault();
    e.stopPropagation();
  }, {passive: false});
}
function initButton() {
  document.getElementById('playbutton').addEventListener('click', playbutton);
  document.getElementById('prevbutton').addEventListener('click', prevbutton);
  document.getElementById('nextbutton').addEventListener('click', nextbutton);
  document.getElementById('volmbutton').addEventListener('click', volmbutton);
  document.getElementById('volpbutton').addEventListener('click', volpbutton);
}

function playercommand(cmd){
	xhr = new XMLHttpRequest();
	xhr.open("POST","/",true);
	xhr.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
	xhr.send(cmd+"=1");
}
function playbutton(){
  var btn=document.getElementById('playbutton');
	if(btn.getAttribute("class")=="stopped") {
	  btn.setAttribute("class", "playing");
	  playercommand("start");
	  return;
	}
	if(btn.getAttribute("class")=="playing") {
	  btn.setAttribute("class", "stopped");
	  playercommand("stop");
	}
}
function prevbutton(){
	playercommand("prev");
}
function nextbutton(){
	playercommand("next");
}
function volmbutton(){
	playercommand("volm");
}
function volpbutton(){
	playercommand("volp");
}
