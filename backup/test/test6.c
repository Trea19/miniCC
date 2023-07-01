int main(){
	int a[10];
	int i = 0;
	int j = 0;
	int tmp;
	while (i < 10) {
		a[i] = read();
		i = i + 1;
	}
	
	// bubble sort
	i = 0;
	while (i < 10){
		j = 0;
		while (j < 10 - i - 1){
			if (a[j] > a[j + 1]){
				tmp = a[j];
				a[j] = a[j + 1];
				a[j + 1] = tmp;
			}
			j = j + 1;
		}
		i = i + 1;
	}
	
	i = 0;
	while (i < 10) {
		write(a[i]);
		i = i + 1;
	}
	
    return 0;
}
