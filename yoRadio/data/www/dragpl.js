let dragged;
let id;
let index;
let indexDrop;
let list;

document.addEventListener("dragstart", ({target}) => {
	dragged = target.parentNode;
	id = target.parentNode.id;
	list = target.parentNode.parentNode.children;
	for(let i = 0; i < list.length; i += 1) {
		if(list[i] === dragged){
			index = i;
		}
	}
});

document.addEventListener("dragover", (event) => {
  event.preventDefault();
});

document.addEventListener("drop", ({target}) => {
	if(target.parentNode.className == "pleitem" && target.parentNode.id !== id) {
		dragged.remove( dragged );
		for(let i = 0; i < list.length; i += 1) {
			if(list[i] === target.parentNode){
				indexDrop = i;
			}
		}
		if(index > indexDrop) {
			target.parentNode.before( dragged );
		} else {
			target.parentNode.after( dragged );
		}
		let items=document.getElementById('pleditorcontent').getElementsByTagName('li');
		for (let i = 0; i <= items.length-1; i++) {
			items[i].getElementsByTagName('span')[0].innerText=("00"+(i+1)).slice(-3);
		}
	}
});
