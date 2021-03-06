#include <nan.h>
#include <string.h>
#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif // win32

extern "C" {
  #include <git2.h>
  {% each cDependencies as dependency %}
    #include <{{ dependency }}>
  {% endeach %}
}

#include <iostream>
#include "../include/nodegit.h"
#include "../include/lock_master.h"
#include "../include/functions/copy.h"
#include "../include/{{ filename }}.h"
#include "nodegit_wrapper.cc"

{% each dependencies as dependency %}
  #include "{{ dependency }}"
{% endeach %}

using namespace v8;
using namespace node;
using namespace std;


// generated from struct_content.cc
{{ cppClassName }}::{{ cppClassName }}() : NodeGitWrapper<{{ cppClassName }}Traits>(NULL, true, v8::Local<v8::Object>())
{
  {% if ignoreInit == true %}
  this->raw = new {{ cType }};
  {% else %}
    {% if isExtendedStruct %}
      {{ cType }}_extended wrappedValue = {{ cType|upper }}_INIT;
      this->raw = ({{ cType }}*) malloc(sizeof({{ cType }}_extended));
      memcpy(this->raw, &wrappedValue, sizeof({{ cType }}_extended));
    {% else %}
      {{ cType }} wrappedValue = {{ cType|upper }}_INIT;
      this->raw = ({{ cType }}*) malloc(sizeof({{ cType }}));
      memcpy(this->raw, &wrappedValue, sizeof({{ cType }}));
    {% endif %}
  {% endif %}

  this->ConstructFields();
}

{{ cppClassName }}::{{ cppClassName }}({{ cType }}* raw, bool selfFreeing, v8::Local<v8::Object> owner)
 : NodeGitWrapper<{{ cppClassName }}Traits>(raw, selfFreeing, owner)
{
  this->ConstructFields();
}

{{ cppClassName }}::~{{ cppClassName }}() {
  {% each fields|fieldsInfo as field %}
    {% if not field.ignore %}
      {% if not field.isEnum %}
        {% if field.isCallbackFunction %}
          if (this->{{ field.name }}.HasCallback()) {
            {% if isExtendedStruct %}
              (({{ cType }}_extended *)this->raw)->payload = NULL;
            {% else %}
              this->raw->{{ fields|payloadFor field.name }} = NULL;
            {% endif %}
          }
        {% elsif field.hasConstructor |or field.isLibgitType %}
          this->{{ field.name }}.Reset();
        {% endif %}
      {% endif %}
    {% endif %}
  {% endeach %}
}

void {{ cppClassName }}::ConstructFields() {
  {% each fields|fieldsInfo as field %}
    {% if not field.ignore %}
      {% if not field.isEnum %}
        {% if field.hasConstructor |or field.isLibgitType %}
          v8::Local<Object> {{ field.name }}Temp = Nan::To<v8::Object>({{ field.cppClassName }}::New(
            {%if not field.cType|isPointer %}&{%endif%}this->raw->{{ field.name }},
            false
          )).ToLocalChecked();
          this->{{ field.name }}.Reset({{ field.name }}Temp);

        {% elsif field.isCallbackFunction %}

          // Set the static method call and set the payload for this function to be
          // the current instance
          this->raw->{{ field.name }} = NULL;
          {% if isExtendedStruct  %}
            (({{ cType }}_extended *)this->raw)->payload = (void *)this;
          {% else %}
            this->raw->{{ fields|payloadFor field.name }} = (void *)this;
          {% endif %}
        {% elsif field.payloadFor %}

          v8::Local<Value> {{ field.name }} = Nan::Undefined();
          this->{{ field.name }}.Reset({{ field.name }});
        {% endif %}
      {% endif %}
    {% endif %}
  {% endeach %}
}

void {{ cppClassName }}::InitializeComponent(Local<Object> target, nodegit::Context *nodegitContext) {
  Nan::HandleScope scope;

  Local<External> nodegitExternal = Nan::New<External>(nodegitContext);
  Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(JSNewFunction, nodegitExternal);

  tpl->InstanceTemplate()->SetInternalFieldCount(1);
  tpl->SetClassName(Nan::New("{{ jsClassName }}").ToLocalChecked());

  {% each fields as field %}
    {% if not field.ignore %}
    {% if not field | isPayload %}
      Nan::SetAccessor(tpl->InstanceTemplate(), Nan::New("{{ field.jsFunctionName }}").ToLocalChecked(), Get{{ field.cppFunctionName}}, Set{{ field.cppFunctionName}}, nodegitExternal);
    {% endif %}
    {% endif %}
  {% endeach %}

  InitializeTemplate(tpl);

  v8::Local<Function> constructor_template = Nan::GetFunction(tpl).ToLocalChecked();
  nodegitContext->SaveToPersistent("{{ cppClassName }}::Template", constructor_template);
  Nan::Set(target, Nan::New("{{ jsClassName }}").ToLocalChecked(), constructor_template);
}

{% partial fieldAccessors . %}

// force base class template instantiation, to make sure we get all the
// methods, statics, etc.
template class NodeGitWrapper<{{ cppClassName }}Traits>;
