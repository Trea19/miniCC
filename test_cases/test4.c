int main(){
    int a, b;
    a = read();
    b = read();
    
    if (a == b)
    	write(1);
    else {
    	if (a > b)
    	    write(2);
    	else 
    	    write(3);
    }
    
    if (a <= b)
    	write(4);
    
    if (a != b)
    	write(5);
    	
    return 0;
}

