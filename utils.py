#!/usr/bin/env python
# _*_ coding: utf-8 _*_

# This module contains various general utility functions
import json
import nltk
import string
import urllib2
import re
from bs4 import BeautifulSoup
from ssl import SSLError
import socket
import httplib
import urlparse

socket.setdefaulttimeout(0.5)
crawlLogFile = open('crawler.log','a')
# load stopWord
stopwordFile = open('stopword','r')
stopwordDict = {}
for line in stopwordFile:
	stopwordDict[line.strip()] = True
	
# filter non-english term
def non_ascii_term(term):
    ascii_reg = re.compile(r'^[\x00-\x7F]+$')
    if ascii_reg.match(term):
        return True
    else:
        return False

def link_term(term):
	url_reg = re.compile(r'(https?://)+(\w+\.)+\w+(/\w+)*/*')
	if url_reg.search(term):
		return True
	else:
		return False

def non_ascii_url_term(term):
	ascii_reg = re.compile(r'^[\x00-\x7F]+$')
	url_reg = re.compile(r'(https?://)+(\w+\.)+\w+(/\w+)*/*')
	if ascii_reg.match(term) and not url_reg.search(term):
		return True
	else:
		return False



def unshorten_url(url):
    parsed = urlparse.urlparse(url)
    h = httplib.HTTPConnection(parsed.netloc)
    h.request('HEAD', parsed.path)
    response = h.getresponse()
    if response.status/100 == 3 and response.getheader('Location'):
        return response.getheader('Location')
    else:
        return url
		
def crawlTitleFromUrl(url):
	hdr = {'User-Agent': 'Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.11 (KHTML, like Gecko) Chrome/23.0.1271.64 Safari/537.11',
       'Accept': 'text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8',
       'Accept-Charset': 'ISO-8859-1,utf-8;q=0.7,*;q=0.3',
       'Accept-Encoding': 'none',
       'Accept-Language': 'en-US,en;q=0.8',
       'Connection': 'keep-alive'}
	long_url = unshorten_url(url)
	#print 'URL:',long_url
	try:
		request = urllib2.Request(long_url, headers=hdr)
		connection = urllib2.urlopen(request,timeout=0.5)
		#html = connection.read()
		#m = re.search("<title>.*</title>", html)
		#content = urllib.urlopen('http://www.apple.com/').read()
		soup = BeautifulSoup(connection.read(10240),'html.parser')
		connection.close()
		#nt 'SOUP:',soup
		title = soup.title.string
		title = title.replace('&quot','')
		space_reg = re.compile('\s+')
		term_list = space_reg.split(title)
		title = filter(non_ascii_url_term,term_list)
		#print 'Title:',' '.join(title)
		return ' '.join(title)
		# if m:
			# title = m.group().strip("</title>") 
			# title = title.replace('&quot','')
			# space_reg = re.compile('\s+')
			# term_list = space_reg.split(title)
			# title = filter(non_ascii_url_term,term_list)
			# print 'Title:',' '.join(title)
			# return ' '.join(title)
		# # else:
			# print 'Miss0:',url
			# crawlLogFile.write(url + '\n')
			# return ''
	
		
	except urllib2.HTTPError, e:
		#print e.fp.read()
		print 'Miss1',str(e)
	except urllib2.URLError, e1:
		print 'MIss2',str(e1)
	except SSLError, e2:
		print 'Miss3',str(e2)
	except Exception, e3:
		print 'Miss4',str(e3)
	return ''
# deal the url
def preprocess(tweetText):
	text = tweetText.replace('\n','').replace('\t','')
	if text.find('RT @') >= 0:    # remove additional commentary
		rt_token = text.split('RT ')
		text = ' '.join(rt_token[1:])
	# else if text.find('RT @') == 0:
	# 	text
	# get url title
	#url_reg = re.compile(r'(https?://)+(\w+\.)+(\w+)(/\w+)*/*')
	space_reg = re.compile('\s+')
	term_list = space_reg.split(text.lower())
	# remove stopword,  @username, rt
	filtered_term_list = [term for term in term_list if not term == '' and not term == 'rt' and not stopwordDict.has_key(term) and not term[0]=='@']
	filtered_term_list2 = filter(non_ascii_term, filtered_term_list)
	text = ' '.join(filtered_term_list2)
	url_reg = re.compile(r'(https?://+\w+\.+\w+/\w+)')
	url_list = url_reg.findall(text)
	#url_list = filter(link_term, filtered_term_list2)
	url_text = ''
	for url in url_list:
		# link = crawlTitleFromUrl(url)
		# if link != '':
			# if url_text == '':
				# url_text = link 
			# else:
				# url_text = url_text + ' ' + link
		text = text.replace(url,'')
	# concatenate with url title
	#text = text + ' ' + url_text
	#print 'middle text:',text
	# deal the text
	remove_punctuation_map = dict((ord(char), None) for char in string.punctuation)
	tokens = nltk.word_tokenize(text.lower())
	filtered_tokens = [word.translate(remove_punctuation_map) for word in tokens]

	# stemmer
	porter = nltk.PorterStemmer()
	stem_tokens = [porter.stem(token.encode('utf-8')) for token in filtered_tokens if not token=='' and not token =='rt' and not stopwordDict.has_key(token) and not token.isdigit()]
	
	return ' '.join(stem_tokens)


def extractStatus(statusJson):
	out_str = ''
	try:
		status = json.loads(statusJson)	
		if status.has_key('created_at'):
			if status.has_key('created_at') and status['lang'] == 'en':
				# preprocess
				#print '============================='
				#print 'origin:', status['text'].replace('\n','')
				preprocessed_text = preprocess(status['text'])
				#print 'Final:',preprocessed_text
				if preprocessed_text != '':
					out_str = '%s\t%s\t%s' % (status['id_str'],status['created_at'],preprocessed_text)
	except:
		print 'json format error'
	return out_str



def extractOriginStatus(statusJson):
	out_str = ''
	try:
		status = json.loads(statusJson)	
		if status.has_key('created_at'):
			if status.has_key('created_at') and status['lang'] == 'en':
				#out_str = '%s\t%s\t%s\n' % (status['id_str'],status['created_at'],status['text'].replace('\n','').replace('\t',''))
				out_str = status['text'].replace('\n','').replace('\t','')
	except:
		print 'origin json format error'
	return out_str

if __name__ == '__main__':
	# url = "http://t.co/mvwxaluzm"
	# crawlTitleFromUrl(url)
	text = 'michael jordan’s 6 greatest http://t.co/tT8J5tbFLj moments in the air jordan 6 http://t.co/q4TnlAXRUy via @champssports'
	preprocess(text)

