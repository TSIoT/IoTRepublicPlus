# IoTRepublicPlus
A IoT Manager server implemnted by C++.<br>
OS support:Windows and Linux<br>

Library Dependncy:<br>
curl:https://curl.haxx.se/<br>
jannson:https://jansson.readthedocs.io/en/2.7/index.html<br>
(library jansson has been integrated into source code)

The reserved manager command id as below:
(the proxy releated function is not implemete yet)
<table>
<tr><td colsapn="2">Implemented:
<tr><th>ID<TH>function<th>Manager<th>UI
<tr><td>Dis_All<td>Discover all device<td>Y<td>N
<tr><td>Dis_NPx<td>Discover non-proxied device<td>Y<td>N
</table>

<table>
<tr><td colsapn="2">Reserve:
<tr><th>ID<TH>function<th>Manager<th>UI
<tr><td>Del_Dev<td>Delete device(by IoT Ip)<td>Y<td>N
<tr><td>Prx_Add<td>Add proxied device(by IoT Ip)<td>Y<td>N
<tr><td>Prx_Rmv<td>Remove proxied device(by IoT Ip)<td>Y<td>N
<tr><td>Rel_Req<td>notice UI need to reload device list<td>Y<td>Y
</table>