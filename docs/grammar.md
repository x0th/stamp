The closest approximation of the formal grammar of Stamp. Productions are outlined in bold.

**program**&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&rarr; **statement_list**


**statement_list**&emsp;&emsp;&emsp;&emsp; &rarr; **statement** ; **statement_list**<br>
&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;→

**statement**&emsp;&emsp;&emsp;&emsp;&emsp;&emsp; &rarr; Object **statement_tail**<br>
&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&rarr; fn function_name **function_tail**<br>
&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&rarr; **if_statement**

**statement_tail**&emsp;&emsp;&emsp;&emsp; &rarr; **message_tail**<br>
&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&rarr; store_name = **rhs**<br>
&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&rarr; = **rhs**

**message_tail**&emsp;&emsp;&emsp;&emsp;&ensp; &rarr; . message_name **message_tail**<br>
&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&rarr; operator **message_tail**<br>
&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&rarr; **Object**<br>
&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&rarr; ( **parameters** ) **message_tail**<br>
&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&rarr;

**function_tail**&emsp;&emsp;&emsp;&emsp;&emsp;&rarr; ( **function_signature** ) { **statement_list** }

**function_signature**&emsp;&emsp;&rarr; **function_signature** **function_signature_tail**<br>
&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&rarr;
                     
**param_type**&emsp;&emsp;&emsp;&emsp;&emsp; &rarr; : Object **next_param**<br>
&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&rarr; **next_param**
                     
**next_param**&emsp;&emsp;&emsp;&emsp;&emsp; &rarr; , **function_signature**<br>
&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&rarr;
                     
**parameters**&emsp;&emsp;&emsp;&emsp;&emsp;&ensp;&rarr; **statement_rhs** , **parameters**<br>
                     &emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&rarr;
                     
**rhs**&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&ensp;&rarr; fn **function_tail**<br>
&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&rarr; **statement_rhs**
                     
**statement_rhs**&emsp;&emsp;&emsp;&emsp; &rarr; Object **message_tail**

**if_statement**&emsp;&emsp;&emsp;&emsp;&emsp; &rarr; if ( **statement_rhs** ) { **statement_list** } **if_tail**

**if_tail**&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&ensp;&rarr; else **else_tail**<br>
&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&rarr;

**else_tail**&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&rarr; **if_statement**<br>
&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&rarr; { **statement_list** }
