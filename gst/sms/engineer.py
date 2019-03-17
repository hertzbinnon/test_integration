class Engineer():
	"""
		Focus on arch abstract
	"""
	token_dict={}
	def __init__(self,
				archclass,
				usage=None, # list of usage about video,audio,spu,
				):
		self.usage = usage
		self.data = {}
		if archclass == 'VLC':
			import binding 
			self.name = 'VLC'
			#self['arch'] = binding
		elif archclass == 'GST':
			self.name = 'GST'
			#self['arch']={}
			#self['arch']['gst'] = Gst
			#self['arch']['gobj'] = GObject 
			#GObject.threads_init()
			#Gst.init(None)
		elif archclass == 'FFM':
			self.name = 'FFM'
		else :
			self.name = 'VLC'
			
	def __setitem__(self, key, item):
		self.data[key]=item
	def __getitem__(self,key):
		return self.data[key]
	def get_name(self):
		return self.name


