import java.lang.reflect.*;
public class ShowMethods {
static final String usage =
"usage: \n" +
"ShowMethods qualified.class.name\n" +
"To show all methods in class or: \n" +
"ShowMethods qualified.class.name word\n" +
"To search for methods involving 'word'";
public static void main(String[] args) {
	if(args.length < 1) {
		System.out.println(usage);
		System.exit(0);
	}
       try {

	Class c = Class.forName(args[0]);
	Method[] m = c.getMethods();
	Constructor[] ctor = c.getConstructors();
	if(args.length == 1) {
		for (int i = 0; i < m.length; i++)
			System.out.println(m[i].toString());
		for (int i = 0; i < ctor.length; i++)
			System.out.println(ctor[i].toString());
	}
	else {
		for (int i = 0; i < m.length; i++)
			if(m[i].toString().indexOf(args[1])!= -1)
				System.out.println(m[i].toString());
		for (int i = 0; i < ctor.length; i++)
			if(ctor[i].toString().indexOf(args[1])!= -1)
				System.out.println(ctor[i].toString());
	}
	} catch (ClassNotFoundException e) {
		System.out.println("No such class: " + e);
	}
  }
}
