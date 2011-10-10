/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is pydrone.
 *
 * The Initial Developer of the Original Code is
 * Unbit S.a.s. (09244720018, Italy).
 * Portions created by the Initial Developer are Copyright (C) 2011
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include <Python.h>
#include <jsapi.h>

#if PY_MAJOR_VERSION > 2
#define PYTHREE
#endif


// define the global class
static JSClass global_class = {  
	.name = "global",
	.flags = JSCLASS_GLOBAL_FLAGS,  
	.addProperty = JS_PropertyStub,
	.delProperty = JS_PropertyStub,
	.getProperty = JS_PropertyStub,
	.setProperty = JS_StrictPropertyStub,  
	.enumerate = JS_EnumerateStub,
	.resolve = JS_ResolveStub,
	.convert = JS_ConvertStub,
	.finalize = JS_FinalizeStub,  
	JSCLASS_NO_OPTIONAL_MEMBERS  
};  

// map js exception to python's one
void js_raise_exc(JSContext *context, const char *message, JSErrorReport *report) { 

	if (!report->filename) {
		PyErr_Format(PyExc_ValueError, "%s", message);
	}
	else {

		PyErr_Format(PyExc_ValueError, "%s:%u:%s", report->filename,
			(unsigned int) report->lineno, message);  
	}

	int *error = JS_GetContextPrivate(context); 

	*error = 1;
}  

// convert js object to py
static PyObject *js_to_py(JSContext *context, jsval rval) {
	

	if (JSVAL_IS_STRING(rval)) {
		char *str = JS_EncodeString(context, JSVAL_TO_STRING(rval));
		return PyUnicode_FromString(str);
	}
	else if (JSVAL_IS_INT(rval)) {
		return PyLong_FromLong(JSVAL_TO_INT(rval));
	}
	else if (JSVAL_IS_DOUBLE(rval)) {
		return PyFloat_FromDouble(JSVAL_TO_DOUBLE(rval));
	}
	else if (JSVAL_IS_OBJECT(rval)) {
		JSObject *obj = JSVAL_TO_OBJECT(rval);
		if (JS_IsArrayObject(context, obj)) {
			PyObject *list = PyList_New(0);
			size_t len,i;
			jsval arrayval;
			if (JS_GetArrayLength(context, obj, &len)) {
				for(i=0;i<len;i++) {
					if (JS_GetElement(context, obj, i, &arrayval)) {
						PyObject *list_item = js_to_py(context, arrayval);
						PyList_Append(list, list_item );
						Py_DECREF(list_item);
					}	
				}
			}
			return list;
		}
		else {
			JSObject *iter = JS_NewPropertyIterator(context, obj);
			if (iter) {
				PyObject *dict = PyDict_New();
				jsid p_id;
				while(JS_NextProperty(context, iter, &p_id) == JS_TRUE) {
					if (p_id == JSID_VOID ) break;
					jsval js_key;
					jsval js_value;
					if (JS_IdToValue(context, p_id, &js_key)) {
						PyObject *py_key = js_to_py(context, js_key);
						if (py_key != Py_None) {
							if (JS_GetPropertyById(context, obj, p_id, &js_value)) {
								PyObject *py_value = js_to_py(context, js_value);
								PyDict_SetItem(dict, py_key, py_value);
								Py_DECREF(py_key);
								Py_DECREF(py_value);
							}
						}
					}
				}
				return dict;
			}
		}
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static jsval py_to_js(JSContext *context, PyObject *data) {


#ifndef PYTHREE
	if (PyString_Check(data)) {
		JSString *js_string = JS_NewStringCopyN(context, PyString_AsString(data), PyString_Size(data));
		return STRING_TO_JSVAL(js_string);
	}
	else if (PyUnicode_Check(data)) {
		PyObject *py_str = PyUnicode_AsUTF8String(data);
		JSString *js_string = JS_NewStringCopyN(context, PyString_AsString(py_str), PyString_Size(py_str));
		Py_DECREF(py_str);
		return STRING_TO_JSVAL(js_string);
	}
#else
	if (PyBytes_Check(data)) {
                JSString *js_string = JS_NewStringCopyN(context, PyBytes_AsString(data), PyBytes_Size(data));
                return STRING_TO_JSVAL(js_string);
        }
	else if (PyUnicode_Check(data)) {
		PyObject *py_str = PyUnicode_AsUTF8String(data);
		JSString *js_string = JS_NewStringCopyN(context, PyBytes_AsString(py_str), PyBytes_Size(py_str));
		Py_DECREF(py_str);
		return STRING_TO_JSVAL(js_string);
	}
#endif
	else if (PyFloat_Check(data)) {
		return DOUBLE_TO_JSVAL( PyFloat_AsDouble(data) );
	}
#ifndef PYTHREE
	else if (PyInt_Check(data)) {
		return INT_TO_JSVAL( PyInt_AsLong(data) );
	}
#endif
	else if (PyLong_Check(data)) {
		return INT_TO_JSVAL( PyLong_AsLong(data) );
	}
	else if (PyList_Check(data)) {
		int i;
		JSObject *js_array = JS_NewArrayObject(context, 0, NULL);
		for(i=0;i<PyList_Size(data);i++) {
			jsval js_py_obj = py_to_js(context, PyList_GetItem(data, i) );
			JS_SetElement(context, js_array, i, &js_py_obj);	
		}	
		return OBJECT_TO_JSVAL(js_array);
	}
	else if (PyTuple_Check(data)) {
		int i;
		JSObject *js_array = JS_NewArrayObject(context, 0, NULL);
		for(i=0;i<PyTuple_Size(data);i++) {
			jsval js_py_obj = py_to_js(context, PyTuple_GetItem(data, i) );
			JS_SetElement(context, js_array, i, &js_py_obj);	
		}	
		return OBJECT_TO_JSVAL(js_array);
	}
	else if (PyDict_Check(data)) {
		char *prop_name;
		PyObject *key, *value;
		Py_ssize_t pos = 0;
		JSObject *js_dict = JS_NewObject(context, NULL, NULL, NULL);
		while (PyDict_Next(data, &pos, &key, &value)) {
			PyObject *py_str = NULL;
			prop_name = NULL;
#ifndef PYTHREE
			if (PyString_Check(key)) {
				prop_name = PyString_AsString(key);
			}	
#else
			if (PyBytes_Check(key)) {
				prop_name = PyBytes_AsString(key);
			}	
#endif
			else if (PyUnicode_Check(key)) {
				PyObject *py_str = PyUnicode_AsUTF8String(key);
#ifndef PYTHREE
				prop_name = PyString_AsString(py_str);
#else
				prop_name = PyBytes_AsString(py_str);
#endif
			}

			if (prop_name != NULL) {
				jsval js_dict_item = py_to_js(context, value);
				JS_SetProperty(context, js_dict, prop_name, &js_dict_item);

				if (py_str) Py_DECREF(py_str);
			}
		}
		return OBJECT_TO_JSVAL(js_dict);	
	}

	return JSVAL_NULL;
}

// the core of the module: create a js runtime/context run code and return a python object
static PyObject *pydrone_js(PyObject *self, PyObject *args) {

	// spidermonkey main pointers
	JSRuntime *runtime;
	JSContext *context;
	JSObject  *global;

	// args
	char *script;
	Py_ssize_t script_len;
	PyObject *py_data;

	// script evaluation return value
	jsval rval;

	int error = 0;


	if (!PyArg_ParseTuple(args, "s#O:js", &script, &script_len, &py_data))
		return NULL;

  
	runtime = JS_NewRuntime (1024L*1024L);
	if (!runtime)
		return PyErr_Format(PyExc_SystemError, "unable to initialize JS runtime\n");
	context = JS_NewContext (runtime, 8192);
	if (!context) {
		JS_DestroyRuntime(runtime);
		return PyErr_Format(PyExc_SystemError, "unable to initialize JS context\n");
	}


	// add error as private data in the context
	JS_SetContextPrivate(context, &error);
   
	// is JIT worthy in pydrone ?
	//JS_SetOptions(context, JSOPTION_VAROBJFIX | JSOPTION_JIT | JSOPTION_METHODJIT);  
	JS_SetOptions(context, JSOPTION_VAROBJFIX);  
	JS_SetVersion(context, JSVERSION_LATEST);
	// map the exception handler
	JS_SetErrorReporter(context, js_raise_exc);

	// initialize the global object
	global = JS_NewCompartmentAndGlobalObject(context, &global_class, NULL);

	// add standard classes (like Array)
	JS_InitStandardClasses(context, global);

	// convert the python data to js and map it as "var data"

	jsval js_data = py_to_js(context, py_data);
	JS_SetProperty(context, global, "data", &js_data);

	// evaluate the script
	JSBool jret = JS_EvaluateScript(context, global, script, script_len, "pydrone", 1, &rval);

	// check for exceptions
	if (jret == JS_FALSE || error == 1) {
		JS_DestroyContext(context);
		JS_DestroyRuntime(runtime);
		JS_ShutDown();
		return NULL;	
	}

  

	PyObject *ret = js_to_py(context, rval);

	JS_DestroyContext(context);
	JS_DestroyRuntime(runtime);
	JS_ShutDown();

	return ret;
}

// methods
static PyMethodDef pydrone_methods[] = {
	{"js",  pydrone_js, METH_VARARGS, "run JavaScript code with SpiderMonkey engine"},
	{NULL, NULL, 0, NULL}
};

// init
#ifndef PYTHREE
PyMODINIT_FUNC initpydrone(void) {
	(void) Py_InitModule("pydrone", pydrone_methods);
#else

static struct PyModuleDef moduledef = {
        PyModuleDef_HEAD_INIT,
        "pydrone",
        "the pydrone module",
        -1,
        pydrone_methods,
	NULL,
	NULL,
	NULL,
	NULL,
};


PyMODINIT_FUNC PyInit_pydrone(void) {

	return PyModule_Create(&moduledef);
#endif
}
