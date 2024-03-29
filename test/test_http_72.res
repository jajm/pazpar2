<?xml version="1.0" encoding="UTF-8"?>
<show><status>OK</status>
<activeclients>0</activeclients>
<merged>1</merged>
<total>1</total>
<start>0</start>
<num>1</num>
<hit>
 <md-title>The religious teachers of Greece</md-title>
 <md-date>1972</md-date>
 <md-author>Adam, James</md-author>
 <md-subject>Greek literature</md-subject>
 <md-subject>Philosophy, Ancient</md-subject>
 <md-subject>Greece</md-subject>
 <md-description>Reprint of the 1909 ed., which was issued as the 1904-1906 Gifford lectures</md-description>
 <location id="z3950.indexdata.com/marc"
    name="Index Data MARC test server" checksum="2614320583">
  <md-title>The religious teachers of Greece</md-title>
  <md-date>1972</md-date>
  <md-author>Adam, James</md-author>
  <md-subject>Greek literature</md-subject>
  <md-subject>Philosophy, Ancient</md-subject>
  <md-subject>Greece</md-subject>
  <md-description tag="500">Reprint of the 1909 ed., which was issued as the 1904-1906 Gifford lectures</md-description>
  <md-description tag="504">Includes bibliographical references</md-description>
  <md-test-usersetting>XXXXXXXXXX</md-test-usersetting>
  <md-test-usersetting-2>test-usersetting-2 data: 
        YYYYYYYYY</md-test-usersetting-2>
  <md-subjects>Greek literature</md-subjects>
  <md-subjects>PAZPAR2_NULL_b</md-subjects>
  <md-subjects>PAZPAR2_NULL_c</md-subjects>
  <md-subjects>Philosophy, Ancient</md-subjects>
  <md-subjects>PAZPAR2_NULL_b</md-subjects>
  <md-subjects>PAZPAR2_NULL_c</md-subjects>
 </location>
 <count>1</count>
 <relevance>291121</relevance>
 <relevance_info>
field=title content=The religious teachers of Greece.;
greece: w[1] += w(6) / (1+log2(1+lead_decay(0.000000) * length(4)));
greece: tf[1] += w[1](6) / length(5) (1.200000);
field=subject content=Greece;
greece: w[1] += w(3) / (1+log2(1+lead_decay(0.000000) * length(0)));
greece: tf[1] += w[1](3) / length(1) (4.200000);
relevance = 0;
idf[1] = log(((1 + total(1))/termoccur(1));
greece: relevance += 100000 * tf[1](4.200000) * idf[1](0.693147) (291121);
score = relevance(291121);
 </relevance_info>
 <recid>content: title the religious teachers of greece author adam james medium book</recid>
</hit>
</show>