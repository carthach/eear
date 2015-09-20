inlets = 1
outlets = 1

pitchs = new Array();

function list(){
	
	if(arguments.length) // bail if no arguments
	{
		// parse arguments
		
		var pitch = arguments[0];
		var velocity = arguments[1];
		var idxToRemove = -1;
		if(velocity==0){
			for( var i = 0 ; i < pitchs.length ; i++){
				if(pitchs[i] == pitch){
					idxToRemove = i;
					break;
					
				} 
			}
			
			if (idxToRemove > -1) {
	    		pitchs.splice(idxToRemove, 1);
			}
			
		
		}
		else{
			pitchs.push(pitch);
			}
		
	}
	
}

function clear(){
	for( var i = 0 ; i < pitchs.length ; i++){
				
					outlet(0,pitchs[i],0);
					
					
				
			}


			pitchs = new Array();
	
	}
