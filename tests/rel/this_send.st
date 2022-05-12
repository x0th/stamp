Dog = Object^;
Dog barks = false;
Dog is_barking = fn() { this.barks }
Dog.is_barking();
