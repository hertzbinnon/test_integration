use std::fmt::Display;

trait Comparable {
  fn compare(&self, object: &Self) -> i8;
}

impl Comparable for f64 {
  fn compare(&self, object: &f64) -> i8 {
    if &self > &object { 1 }
    else if &self == &object { 0 }
    else { -1 }
  }
}

fn max<T: Comparable>(array: &[T]) -> &T {
  let mut max_index = 0;
  let mut i = 1;
  while i < array.len() {
    if array[i].compare(&array[max_index]) > 0 {
          max_index = i;
  }
    i += 1;
  }
  &array[max_index]
}

fn longest_with_an_announcement<'a, T>(x: &'a str, y: &'a str, ann: T) -> &'a str
    where T: Display
{
    println!("Announcement! {}", ann);
    if x.len() > y.len() {
        x
    } else {
        y
    }
}

fn main(){
    for i in 0i32..10 {
      print!("{} ", i);
    }
    println!("");
    enum OptionalI32 {
      AnI32(i32),
      Nothing,
    }
    let foo = OptionalI32::AnI32(1);
    match foo {
      OptionalI32::AnI32(n) => println!("it’s an i32: {}", n),
      OptionalI32::Nothing  => println!("it’s nothing!"),
    }
    struct FooBar { x: i32, y: OptionalI32 }
    let bar = FooBar { x: 15, y: OptionalI32::AnI32(32) };

    match bar {
      FooBar { x: 0, y: OptionalI32::AnI32(0) } =>
          println!("The numbers are zero!"),
      FooBar { x: n, y: OptionalI32::AnI32(m) } if n == m =>
          println!("The numbers are the same"),
      FooBar { x: n, y: OptionalI32::AnI32(m) } =>
          println!("Different numbers: {} {}", n, m),
      FooBar { x: _, y: OptionalI32::Nothing  }  =>
          println!("The second number is Nothing!"),
    }
    let s1:FooBar;//= "yes i do";
    let s2 = "no i can`t";
    //longest_with_an_announcement(s1,s2,std::fmt::Display);
    let a=[1.0, 2.0, 3.0, 4.0, 5.0];
    println!("generic T is {}",max(&a));
}
