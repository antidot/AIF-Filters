#!/usr/local/afs7/bin/python3.3
# -*- coding: utf-8 -*-
"""
AFS
(C) 2013 Antidot

 Twitter Tweet Load.
"""
__version__ = "1.0 (2013-12-26)"
__author__  = "filter template generator"

import sys
import logging

import afs.paf
import afs.paf.status as status
import afs.paf.layer_type as layer_type
import afs.paf.event as event

#Additionnal lib
import os
import hashlib
from twython import Twython
from lxml import etree
import json
from time import strptime

class TwitterTweetLoadFilter(afs.paf.GeneratorFilter):
    def __init__(self, conf, handle):
        self.access_secret = None
        self.consumer_key = None
        self.output_layer = None
        self.consumer_secret = None
        self.access_key = None
        self.users = None
        self.max = None
        afs.paf.GeneratorFilter.__init__(self, conf, handle)

    def init(self):
        self._logger.info("Initialize filter", extra={"severityno":9})

        # Access secret value
        if not self._configuration.has_arg("access_secret"):
            raise ValueError("Parameter access_secret is mandatory")
        self.access_secret = self._configuration.get_string("access_secret")
        self._handle.log(event.INFO, "Parameter access_secret has value: "+str(self.access_secret), event.DEBUG)

        # List of Twitter users
        if not self._configuration.has_arg("users"):
            raise ValueError("Parameter users is mandatory")
        self.users = self._configuration.get_string_list("users")
        self._handle.log(event.INFO, "Parameter users has value: "+str(self.users), event.DEBUG)

        # Consumer secret value
        if not self._configuration.has_arg("consumer_secret"):
            raise ValueError("Parameter consumer_secret is mandatory")
        self.consumer_secret = self._configuration.get_string("consumer_secret")
        self._handle.log(event.INFO, "Parameter consumer_secret has value: "+str(self.consumer_secret), event.DEBUG)

        # Consumer key value
        if not self._configuration.has_arg("consumer_key"):
            raise ValueError("Parameter consumer_key is mandatory")
        self.consumer_key = self._configuration.get_string("consumer_key")
        self._handle.log(event.INFO, "Parameter consumer_key has value: "+str(self.consumer_key), event.DEBUG)

        # Access key value
        if not self._configuration.has_arg("access_key"):
            raise ValueError("Parameter access_key is mandatory")
        self.access_key = self._configuration.get_string("access_key")
        self._handle.log(event.INFO, "Parameter access_key has value: "+str(self.access_key), event.DEBUG)

        # The layer where to store tweets
        self.output_layer = self._configuration.get_output_type(layer_type.CONTENTS)
        self._handle.log(event.INFO, "Parameter output_layer has value: "+str(self.output_layer) if self.output_layer is None else layer_type.from_number(self.output_layer), event.DEBUG)
        
        # Max number of tweets to generate                        
        if self._configuration.has_arg("max"):
            self.max = self._configuration.get_int("max")
        else:
            self.max = 100
        self._handle.log(event.INFO, "Parameter max has value: "+str(self.max), event.DEBUG)


    def generate(self):
        self._logger.info("Generate step", extra={"severityno":9})

        # Now generate documents
        self.twythonConnection=self.authentication()
        
        for user in self.users:
            #Get tweets from each user's twitter timeline
            self._logger.info("T_twitter_tweet_load::generate() Get timeline for %s" % user, extra={"severityno":1})
            
            self.timeline=self.twythonConnection.get_user_timeline(screen_name=user, count=self.max)

            #Create new document for each tweet of the current user (stored in x_id)
            for tweet in self.timeline:
                self.md5_id=hashlib.md5(bytes(tweet['text'],'utf-8')).hexdigest()
                self._logger.info("T_twitter_tweet_load::generate() URI created : %s" % self.md5_id, extra={"severityno":1})
                self.newdoc = self._handle.new_document("urn:afs:%s" % self.md5_id)
                tweetContent=""
                #Root node ...
                root = etree.Element('tweet')
                tweetId=tweet['id']
                root.set("id",str(tweetId))
             
                #Author node
                authorElt = etree.SubElement(root, 'author')

                #Author sub elements
                userScreenNameElt = etree.SubElement(authorElt, 'userScreenName')
                userScreenNameElt.text=tweet['user']['screen_name'].replace('"','')

                userImgElt = etree.SubElement(authorElt, 'userImg')
                userImgElt.text = tweet['user']['profile_image_url'].replace('"','')
                
                userNameElt = etree.SubElement(authorElt, 'userName')
                userNameElt.text = tweet['user']['name'].replace('"','')


                userDescriptionElt = etree.SubElement(authorElt, 'userDescription')
                userDescriptionValue = tweet['user']['description'].replace('"','')
                if userDescriptionValue != 'null' :
                    userDescriptionElt.text = userDescriptionValue
                else:
                    pass

                #Publication Date (re-formatted: from "18 Apr 2013" to "2013/04/18")
                dateElt = etree.SubElement(root, 'date')
                dateEltValue=json.dumps(tweet['created_at'], indent=4, separators=(',', ': ')).split(" ")
                dateMonthNumber=strptime(dateEltValue[1],'%b').tm_mon                
                dateElt.text="/".join([dateEltValue[5].replace('"',''),str(dateMonthNumber),dateEltValue[2]])

                #Hashtags section
                hashtagsElt = etree.SubElement(root, 'hashtags')
                hashtagsList = tweet['entities']['hashtags']
                hashtagCount=0
                if len(hashtagsList) > 0 :
                    for h in hashtagsList : 
                        hElt = etree.SubElement(hashtagsElt, 'hashtag')
                        hElt.text = h['text'].replace('"','')
                        hashtagCount+=1
                else:
                    pass                
                hashtagsElt.set("number", str(hashtagCount))
                
                #User mentions section
                userMentionsElt = etree.SubElement(root, 'userMentions')
                umList = tweet['entities']['user_mentions']
                umCount=0
                if len(umList) > 0 :
                    for um in umList : 
                        uElt = etree.SubElement(userMentionsElt, 'userMention')
                        uElt.text = um['screen_name'].replace('"','')
                        umCount+=1
                else:
                    pass                
                userMentionsElt.set("number", str(umCount))
            
                # Tweet text section
                textElt = etree.SubElement(root, 'text')
                textElt.text = tweet['text'].replace('"','')
                
                tweetContent+=etree.tostring(root, xml_declaration=True, pretty_print=True).decode('utf-8')
               
                self.newdoc.set_layer(tweetContent, layer_type.CONTENTS)
                self.newdoc.set_x_id(user)
                self._handle.send_if_new(self.newdoc)
                
    def authentication(self,):
        #Connect to api with authentication key get on twitter dev website        
        self._logger.info("T_twitter_tweet_load::authenticate() Open connection to Twitter ... ",extra={"severityno":1})        
        self.api = Twython(self.consumer_key, self.consumer_secret, self.access_key, self.access_secret)
        self._logger.info("T_twitter_tweet_load::authenticate() ... connection opened ",extra={"severityno":1})
        return self.api

if __name__ == '__main__':
    afs.paf.start_filter(TwitterTweetLoadFilter, sys.argv)
