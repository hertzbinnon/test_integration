//: BeanDumper.java
// A method to introspect a Bean
import java.beans.*;
import java.lang.reflect.*;
public class BeanDumper {
public static void dump(Class bean){
	BeanInfo bi = null;
	try {
	bi = Introspector.getBeanInfo(bean, java.lang.Object.class);
	} catch(IntrospectionException ex) {
		System.out.println("Couldn't introspect " +
		bean.getName());
		System.exit(1);
	}
	PropertyDescriptor[] properties = bi.getPropertyDescriptors();
	for(int i = 0; i < properties.length; i++) {
		Class p = properties[i].getPropertyType();
		System.out.println("Property type:\n " + p.getName());
		System.out.println("Property name:\n " +properties[i].getName());
		Method readMethod = properties[i].getReadMethod();
		if(readMethod != null)
			System.out.println("Read method:\n " +readMethod.toString());
		Method writeMethod = properties[i].getWriteMethod();
		if(writeMethod != null)
		System.out.println("Write method:\n " + writeMethod.toString());
		System.out.println("====================");
	}
	System.out.println("Public methods:");
	MethodDescriptor[] methods = bi.getMethodDescriptors();
	for(int i = 0; i < methods.length; i++)
		System.out.println(methods[i].getMethod().toString());
	System.out.println("======================");
	System.out.println("Event support:");
	EventSetDescriptor[] events = bi.getEventSetDescriptors();
	for(int i = 0; i < events.length; i++) {
		System.out.println("Listener type:\n " +
		events[i].getListenerType().getName());
		Method[] lm = events[i].getListenerMethods();
		for(int j = 0; j < lm.length; j++)
			System.out.println("Listener method:\n " + lm[j].getName());
		MethodDescriptor[] lmd = events[i].getListenerMethodDescriptors();
		for(int j = 0; j < lmd.length; j++)
			System.out.println("Method descriptor:\n " +lmd[j].getMethod().toString());
		Method addListener = events[i].getAddListenerMethod();
		System.out.println("Add Listener Method:\n " + addListener.toString());
		Method removeListener = events[i].getRemoveListenerMethod();
		System.out.println("Remove Listener Method:\n " + removeListener.toString());
		System.out.println("====================");
	}
}
// Dump the class of your choice:
public static void main(String[] args) {
	if(args.length < 1) {
		System.err.println("usage: \n" +
			"BeanDumper fully.qualified.class");
		System.exit(0);
	}
	Class c = null;
	try {
		c = Class.forName(args[0]);
	} catch(ClassNotFoundException ex) {
		System.err.println(
			"Couldn't find " + args[0]);
		System.exit(0);
	}
	dump(c);
	}
} ///:~
