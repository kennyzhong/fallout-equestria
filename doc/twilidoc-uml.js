// Generated by CoffeeScript 1.3.3
(function() {

  $(document).ready(function() {
    var GenerateHierarchyUml, generate_uml, get_project_type, uml, uml_height, visibility_string2uml;
    uml = Joint.dia.uml;
    uml_height = 0;
    visibility_string2uml = function(obj) {
      switch (obj.visibility) {
        case 'public':
          return '+';
        case 'protected':
          return '#';
        case 'private':
          return '-';
        default:
          return '?';
      }
    };
    get_project_type = function(name) {
      var type, _i, _len, _ref;
      _ref = project.types;
      for (_i = 0, _len = _ref.length; _i < _len; _i++) {
        type = _ref[_i];
        if (type.name === name) {
          return type;
        }
      }
      return null;
    };
    generate_uml = function(type, visibility, start_pos) {
      var ancestor, ancestor_pos, ancestor_type, attribute, attributes, color, default_pos, height, inherits_object, method, methods, object, position, str, tmp, tmp2, width, _i, _j, _k, _len, _len1, _len2, _ref, _ref1, _ref2;
      color = 'yellow';
      methods = [];
      attributes = [];
      object = null;
      default_pos = {
        x: 0,
        y: 0
      };
      position = start_pos == null ? default_pos : start_pos;
      width = 100;
      height = 50;
      if (visibility === 'protected') {
        color = 'lightgreen';
      } else if (visibility === 'private') {
        color = 'lightblue';
      }
      _ref = type.methods;
      for (_i = 0, _len = _ref.length; _i < _len; _i++) {
        method = _ref[_i];
        str = visibility_string2uml(method);
        str += ' ' + method.name + '(' + method.params + ') -> ' + method.return_type;
        methods.push(str);
        if (str.length * 5 > width) {
          width = 10 + str.length * 5;
        }
        height += 13;
      }
      _ref1 = type.attributes;
      for (_j = 0, _len1 = _ref1.length; _j < _len1; _j++) {
        attribute = _ref1[_j];
        str = visibility_string2uml(attribute);
        str += ' ' + attribute.name + ' -> ' + attribute.type;
        attributes.push(str);
        if (str.length * 5 > width) {
          width = 10 + str.length * 5;
        }
        height += 13;
      }
      object = uml.Class.create({
        label: type.name,
        shadow: true,
        attrs: {
          fill: color
        },
        labelAttrs: {
          'font-weight': 'bold'
        },
        methods: methods,
        attributes: attributes,
        rect: {
          x: position.x,
          y: position.y,
          width: width,
          height: height
        },
        interactive: false
      });
      object.draggable(false);
      if (position.y + height + 10 > uml_height) {
        uml_height = position.y + height + 10;
      }
      position.x += width + 50;
      tmp = position.x;
      _ref2 = type.ancestors;
      for (_k = 0, _len2 = _ref2.length; _k < _len2; _k++) {
        ancestor = _ref2[_k];
        ancestor_type = get_project_type(ancestor.name);
        ancestor_pos = position;
        if (ancestor_type !== null) {
          tmp2 = position.y;
          inherits_object = generate_uml(ancestor_type, ancestor.visibility, ancestor_pos);
          position.y = tmp2;
          object.joint(inherits_object, uml.generalizationArrow);
          console.log(inherits_object);
          position.x = tmp;
          position.y += inherits_object.getBBox().height + 10;
        }
      }
      return object;
    };
    GenerateHierarchyUml = function(id, classname) {
      var joint, klass;
      joint = Joint.paper(id, 1, 1);
      uml_height = 0;
      klass = get_project_type(classname);
      if (klass !== null) {
        generate_uml(klass, 'public');
      } else {
        alert("UML: Couldn't find type '" + classname + "'");
      }
      return joint.setSize($('#' + id).parent().width(), uml_height);
    };
    window.get_project_type = get_project_type;
    window.uml = {};
    return window.uml.generate_hierarchy = GenerateHierarchyUml;
  });

}).call(this);