class ShowMsgBox {
	public static void main(String [] args) {
	ShowMsgBox app = new ShowMsgBox();
	app.ShowMessage("Generated with JNI");
	}
	private native void ShowMessage(String msg);
	static {
		System.loadLibrary("libc.so");
	}
}
