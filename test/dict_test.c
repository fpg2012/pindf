#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "../pdf/obj.h"
#include "../container/simple_vector.h"
#include "../container/uchar_str.h"

int main(void) {
    // Create a dictionary
    pindf_pdf_dict dict;
    dict.keys = pindf_vector_new(8, sizeof(pindf_pdf_obj*));
    dict.values = pindf_vector_new(8, sizeof(pindf_pdf_obj*));

    // Create some test objects
    pindf_pdf_obj *int_obj1 = pindf_pdf_obj_new(PINDF_PDF_INT);
    int_obj1->content.num = 42;

    pindf_pdf_obj *int_obj2 = pindf_pdf_obj_new(PINDF_PDF_INT);
    int_obj2->content.num = 100;

    pindf_pdf_obj *str_obj = pindf_pdf_obj_new(PINDF_PDF_LTR_STR);
    str_obj->content.ltr_str = pindf_uchar_str_from_cstr("Hello", 5);

    // Test 1: Set new key
    printf("Test 1: Set new key 'foo'\n");
    pindf_dict_set_value2(&dict, "foo", int_obj1);

    // Get it back
    pindf_pdf_obj *val = pindf_dict_getvalue2(&dict, "foo");
    assert(val != NULL);
    assert(val->obj_type == PINDF_PDF_INT);
    assert(val->content.num == 42);
    printf("  OK: foo = %d\n", val->content.num);

    // Test 2: Set another new key
    printf("Test 2: Set new key 'bar'\n");
    pindf_dict_set_value2(&dict, "bar", int_obj2);

    val = pindf_dict_getvalue2(&dict, "bar");
    assert(val != NULL);
    assert(val->obj_type == PINDF_PDF_INT);
    assert(val->content.num == 100);
    printf("  OK: bar = %d\n", val->content.num);

    // Test 3: Replace existing key
    printf("Test 3: Replace key 'foo' with string\n");
    pindf_dict_set_value2(&dict, "foo", str_obj);

    val = pindf_dict_getvalue2(&dict, "foo");
    assert(val != NULL);
    assert(val->obj_type == PINDF_PDF_LTR_STR);
    assert(val->content.ltr_str != NULL);
    assert(pindf_uchar_str_cmp3(val->content.ltr_str, "Hello") == 0);
    printf("  OK: foo is now string 'Hello'\n");

    // Test 4: Verify 'bar' still exists
    val = pindf_dict_getvalue2(&dict, "bar");
    assert(val != NULL);
    assert(val->obj_type == PINDF_PDF_INT);
    assert(val->content.num == 100);
    printf("  OK: bar still = %d\n", val->content.num);

    // Test 5: Set with same value (should not destroy)
    printf("Test 5: Set 'foo' with same value\n");
    pindf_dict_set_value2(&dict, "foo", str_obj); // same object

    val = pindf_dict_getvalue2(&dict, "foo");
    assert(val == str_obj); // should be same pointer
    printf("  OK: pointer unchanged\n");

    // Test 6: Set with byte string key
    printf("Test 6: Set with byte string key\n");
    pindf_pdf_obj *int_obj3 = pindf_pdf_obj_new(PINDF_PDF_INT);
    int_obj3->content.num = 999;
    pindf_dict_set_value(&dict, (const uchar*)"baz", 3, int_obj3);

    val = pindf_dict_getvalue(&dict, (const uchar*)"baz", 3);
    assert(val != NULL);
    assert(val->obj_type == PINDF_PDF_INT);
    assert(val->content.num == 999);
    printf("  OK: baz = %d\n", val->content.num);

    // Clean up - dictionary owns the objects, but we need to destroy the dictionary
    printf("Cleaning up...\n");
    pindf_pdf_dict_destory(&dict);

    printf("All tests passed!\n");
    return 0;
}