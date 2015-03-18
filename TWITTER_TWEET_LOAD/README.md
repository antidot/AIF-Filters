## TWITTER TWEET LOAD

###### Author: _RBE_
###### Version: _1.0_
###### Date: _2013-12-27_

**Description**: AFSv7 PaF generator filter used to get tweets from given users timelines, and generate a new document for each tweet

**Features**:

	- Relies on [Twython](http://twython.readthedocs.org/en/latest/) library (Git repository: git://github.com/ryanmcgrath/twython.git)
	
	- Works with the new Twitter API v1.1
	
	- For each tweet, the current user name is stored in the x_id identifier

**Parameters**:

	- consumer_key / consumer_secret / access_token / access_secret : OAuth settings and Access token parameters (to sign requests with your own Twitter account) from  [http://dev.twitter.com/](http://dev.twitter.com/)
	
	- users : List of Twitter users used 
	
	- max (default : 100) : Max number of tweets to retrieve per user
	
	- output_layer (optional, default : CONTENTS, Layer) : the output layer

**Requirements** : 

	- Setuptools 3 : python3-setuptools
	
	- Requests : antidot-python33-requests
	
	- Requests Oauthlib : https://pypi.python.org/packages/source/r/requests-oauthlib/requests-oauthlib-0.3.3.tar.gz
	
	- OAuthlib :  https://pypi.python.org/packages/source/o/oauthlib/oauthlib-0.6.0.tar.gz


#### Twython installation with python 3.3 (Antidot package) and dependencies 
##### Twython 
	
	git clone git://github.com/ryanmcgrath/twython.git
	cd twython/
	/usr/local/afs7/contrib/python/3.3/bin/python3.3 setup.py install
	

##### Setuptools 3
	
	apt-get install python3-setuptools
	

##### Requests
	
	apt-get install antidot-python33-requests
	

##### Requests Oauthlib
	
	wget https://pypi.python.org/packages/source/r/requests-oauthlib/requests-oauthlib-0.3.3.tar.gz --no-check-certificate
	tar xvzf requests-oauthlib-0.3.3.tar.gz
	cd requests-oauthlib-0.3.3/
	/usr/local/afs7/contrib/python/3.3/bin/python3.3 setup.py install
	

##### OAuthlib
	
	wget https://pypi.python.org/packages/source/o/oauthlib/oauthlib-0.6.0.tar.gz --no-check-certificate
	tar xvzf oauthlib-0.6.0.tar.gz
	cd oauthlib-0.6.0 
	/usr/local/afs7/contrib/python/3.3/bin/python3.3 setup.py install
	
