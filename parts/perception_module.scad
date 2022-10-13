include <config.scad>
include <frame.scad>

standoff_w = 9.0;
standoff_h = 4.0;
support_w = 6.0;
support_h = 4.0;

fcu_w = 60.0;
fcu_d = 40.0;
fcu_h = 25.0;
fcu_mount_w = 56.0;
fcu_mount_d = 36.0;
fcu_standoff_h = standoff_h + 3;
fcu_support_h = standoff_h - 2;

nuc_mount_w = 95.0;
nuc_mount_d = 90.4;

stereo_baseline = 70.0;
pitch_frame_w = stereo_baseline + standoff_h * 4 + 30;

module lipo_battery(c=[0.4, 0.4, 0.4]) {
  color(c)
    cube([batt_w, batt_d, batt_h], center=true);
}

module encoder_board() {
  w = 22;
  d = 28;
  h = 1.5;

  chip_w = 5;
  chip_d = 4;
  chip_h = 0.5;

  hole_w = 2.5;
  mount_w = 18;
  mount_d = 11;

  offset_y = d / 2;

  translate([0, -offset_y / 2, 0])
  difference() {
    union() {
      // Body
      color([0, 1, 0])
        translate([0, 0, h / 2])
          cube([w, d, h], center=true);

      // Encoder chip
      color([0, 0, 0])
        translate([0, d / 2 - 7, h])
          cube([chip_w, chip_d, chip_h], center=true);
    }

    // Mount holes
    translate([mount_w / 2, (mount_d + offset_y) / 2, h / 2])
      cylinder(r=hole_w / 2, h=h + 0.1, center=true);
    translate([-mount_w / 2, (mount_d + offset_y) / 2, h / 2])
      cylinder(r=hole_w / 2, h=h + 0.1, center=true);
    translate([mount_w / 2, (-mount_d + offset_y) / 2, h / 2])
      cylinder(r=hole_w / 2, h=h + 0.1, center=true);
    translate([-mount_w / 2, (-mount_d + offset_y) / 2, h / 2])
      cylinder(r=hole_w / 2, h=h + 0.1, center=true);

    // Solder holes
    translate([-(7 * 2.54) / 2, -d / 2 + 1.5, 0])
      for (i = [0:7])
        translate([i * 2.54, 0, h / 2])
          cylinder(r=0.5, h=h + 0.1, center=true);
  }
}


module stack_spacer(h, nut_counter_sink=1) {
  screw_size = M3_screw_w;
  screw_hsize = screw_size / 2.0;
  nut_w = M3_nut_w;
  nut_h = M3_nut_h;
  tol = 0.2;

  translate([0.0, 0.0, h / 2.0]) {
    difference() {
      // Spacer body
      cylinder(h=h, r=screw_size + 1.0, center=true);

      // Thread hole
      cylinder(h=h + 0.01, r=screw_hsize + tol, center=true);
      // Nut counter sinks
      if (nut_counter_sink) {
        translate([0.0, 0.0, h / 2.0 - nut_h / 2.0]) {
          cylinder(h=nut_h + 0.01, r=nut_w / 2.0, $fn=6, center=true);
        }
        translate([0.0, 0.0, -h / 2.0 + nut_h / 2.0]) {
          cylinder(h=nut_h + 0.01, r=nut_w / 2.0, $fn=6, center=true);
        }
      }
    }
  }
}

module nut_tool() {
  tol = 0.2;

  // M3 size
  tool_h = 25.0;
  screw_size = 3.0;
  screw_hsize = screw_size / 2.0;
  nut_w = 6.5;

  // M2 size
  // tool_h = 20.0;
  // screw_size = 2.0;
  // screw_hsize = screw_size / 2.0;
  // nut_w = 4.7;

  wing_w = 35.0;
  wing_d = 8.0;
  wing_h = 5.0;

  translate([0.0, 0.0, tool_h / 2.0]) {
    difference() {
      // Tool body
      union() {
        cylinder(h=tool_h, r=screw_hsize + 2.5, center=true);
        translate([0.0, 0.0, -tool_h / 2.0 + wing_h / 2.0])
        cube([wing_w, wing_d, wing_h], center=true);
      }

      // Tool hole
      cylinder(h=tool_h + 0.01, r=nut_w / 2.0, $fn=6, center=true);
    }
  }
}

module m12_lens_mount() {
  w = 16;
  base_h = 4.5;
  mount_color = [0.3, 0.3, 0.3];
  flange_w = 18.0;

  difference() {
    union() {
      // Base
      translate([0.0, 0.0, base_h / 2.0])
        color(mount_color)
          cube([w, w, base_h], center=true);

      // Lens thread body
      translate([0.0, 0.0, base_h + 10.0 / 2.0])
        color(mount_color)
          cylinder(h=10, r=7.5, center=true);
    }

    // Lens thread hole
    translate([0.0, 0.0, base_h / 2.0 + 10.0 / 2.0])
      color(mount_color)
        cylinder(h=14.5 + 0.01, r=6, center=true);
  }

  difference() {
    // Lens mount body
    union() {
      translate([0.0, flange_w / 2.0, base_h / 2.0])
        color(mount_color)
          cylinder(h=base_h, r=4.2 / 2.0, center=true);

      translate([0.0, flange_w / 2.0 - 3.0 / 2.0, base_h / 2.0])
        color(mount_color)
          cube([4.2, 3.0, base_h], center=true);
    }

    // Lens mount hole
    translate([0.0, flange_w / 2.0, base_h / 2.0])
      color(mount_color)
        cylinder(h=base_h + 0.01, r=2.0 / 2.0, center=true);
  }

  difference() {
    // Lens mount body
    union() {
      translate([0.0, -flange_w / 2.0, base_h / 2.0])
        color(mount_color)
          cylinder(h=base_h, r=4.2 / 2.0, center=true);

      translate([0.0, -flange_w / 2.0 + 3.0 / 2.0, base_h / 2.0])
        color(mount_color)
          cube([4.2, 3.0, base_h], center=true);
    }

    // Lens mount hole
    translate([0.0, -flange_w / 2.0, base_h / 2.0])
      color(mount_color)
        cylinder(h=base_h + 0.01, r=2.0 / 2.0, center=true);
  }
}

module board_camera() {
  pcb_width = 30.0;
  pcb_thickness = 1.58;
  pcb_hole = 2.1;
  mount_w = 24.5;
  lens_hole = 2.0;

  difference() {
    // PCB body
    translate([0.0, 0.0, pcb_thickness / 2.0])
      color([0.0, 1.0, 0.0])
        cube([pcb_width, pcb_width, pcb_thickness], center=true);

    // Lens holes
    translate([0.0, 9.0, pcb_thickness / 2.0])
      cylinder(h=pcb_thickness + 0.01, r=lens_hole / 2.0, center=true);
    translate([0.0, -9.0, pcb_thickness / 2.0])
      cylinder(h=pcb_thickness + 0.01, r=lens_hole / 2.0, center=true);


    // Mount holes
    for (i = [1 : 4]) {
      rotate([0.0, 0.0, i * 90.0])
        translate([mount_w / 2.0, mount_w / 2.0, pcb_thickness / 2.0])
          cylinder(h=pcb_thickness + 0.01, r=pcb_hole / 2.0, center=true);
    }
  }

  // Lens mount
  translate([0.0, 0.0, pcb_thickness-0.01])
    m12_lens_mount();
}

module gimbal_imu() {
  w = 20.0;
  d = 17.5;
  h = 4.0;

  difference() {
    color([0, 1, 0]) translate([0, 0, h / 2]) cube([w, d, h], center=true);
  }
}

module mpu6050() {
  w = 21.0;
  d = 15.5;
  h = 4.0;
  mount_w = 15.5;
  mount_r = 3.0;

  rotate(90.0)
    difference() {
      color([0, 1, 0]) translate([0, 0, h / 2]) cube([w, d, h], center=true);

      translate([mount_w / 2, -d / 2 + 2.5, h / 2])
        cylinder(r=3.5 / 2, h=h + 0.1, center=true);
      translate([-mount_w / 2, -d / 2 + 2.5, h / 2])
        cylinder(r=3.5 / 2, h=h + 0.1, center=true);
    }
}


module fcu_frame(show_fcu=0) {
  if (show_fcu) {
    translate([0.0, 0.0, fcu_standoff_h])
      color([0.0, 1.0, 0.0])
        difference() {
          translate([0, 0, fcu_h / 2])
            cube([fcu_w, fcu_d, fcu_h], center=true);

          translate([fcu_mount_w / 2, fcu_mount_d / 2, fcu_h / 2])
            cylinder(r=M2_screw_w / 2, h=fcu_h + 0.1, center=true);
          translate([-fcu_mount_w / 2, fcu_mount_d / 2, fcu_h / 2])
            cylinder(r=M2_screw_w / 2, h=fcu_h + 0.1, center=true);
          translate([fcu_mount_w / 2, -fcu_mount_d / 2, fcu_h / 2])
            cylinder(r=M2_screw_w / 2, h=fcu_h + 0.1, center=true);
          translate([-fcu_mount_w / 2, -fcu_mount_d / 2, fcu_h / 2])
            cylinder(r=M2_screw_w / 2, h=fcu_h + 0.1, center=true);
        }
  }

  difference() {
    union() {
      // Mount point
      frame(30.5, 30.5,
            M3_screw_w, M3_nut_w, M3_nut_h,
            standoff_w, standoff_h -2, standoff_h -2);

      // FCU frame
      frame(fcu_mount_w, fcu_mount_d,
            M2_screw_w, M2_nut_w, M2_nut_h,
            standoff_w - 2.5, fcu_standoff_h, fcu_support_h, 0, 1);

      // Battery frame
      rotate(90)
      frame(batt_frame_d, batt_frame_w, M3_screw_w, M3_nut_w, M3_nut_h,
            standoff_w, standoff_h, fcu_support_h, 0, 1);
    }

    // Holes
    for (i = [0:90:360])
      rotate([0, 0, i])
        translate([30.5 / 2, 30.5 / 2])
          cylinder(r=3.2 / 2.0, h=standoff_h + 0.1);
  }

  // // Fill in the gaps
  // translate([0.0, fcu_mount_d / 2 + 3, standoff_h / 2.0])
  //   cube([fcu_mount_w - M2_screw_w, standoff_w / 2.0, standoff_h], center=true);
  // translate([0.0, -fcu_mount_d / 2 - 3, standoff_h / 2.0])
  //   cube([fcu_mount_w - M2_screw_w, standoff_w / 2.0, standoff_h], center=true);
}

module battery_frame(mount_w, mount_d, show_battery=0) {
  // nb_supports = 4;
  // diff = batt_frame_d / (nb_supports + 1);

  // Lipo battery
  if (show_battery) {
    translate([0.0, 0.0, batt_h / 2.0 + standoff_h])
      rotate([0.0, 0.0, 90.0])
      lipo_battery();
  }

  // Frame
  difference() {
    union() {
      // Battery frame
      frame(batt_frame_w, batt_frame_d, M3_screw_w, M3_nut_w, M3_nut_h,
            standoff_w + 1, standoff_h, standoff_h, 0);

      // Supports
      translate([0, 9, standoff_h / 2])
        cube([batt_frame_w, 3.0, standoff_h], center=true);
      translate([0, -9, standoff_h / 2])
        cube([batt_frame_w, 3.0, standoff_h], center=true);
      translate([0, 18, standoff_h / 2])
        cube([batt_frame_w, 3.0, standoff_h], center=true);
      translate([0, -18, standoff_h / 2])
        cube([batt_frame_w, 3.0, standoff_h], center=true);
      // for (spacing = [diff:diff:batt_frame_d-diff]) {
      //   translate([0, -batt_frame_d / 2 + spacing, standoff_h / 2])
      //     #cube([batt_frame_w, 3.0, standoff_h], center=true);
      // }

      // Battery strap support
      translate([batt_frame_w / 2 - 6, 0, standoff_h / 2])
        cube([3.0, 16, standoff_h], center=true);
      translate([-batt_frame_w / 2 + 6, 0, standoff_h / 2])
        cube([3.0, 16, standoff_h], center=true);

      // Overhangs
      hang_w = 30;
      hang_t = 1.5;
      hang_s = 5;
      translate([0, batt_frame_d / 2 + hang_s, standoff_h / 2])
        cube([hang_w, hang_t, standoff_h], center=true);
      translate([hang_w / 2 - hang_t / 2, batt_frame_d / 2 + hang_s / 2, standoff_h / 2])
        cube([hang_t, hang_s, standoff_h], center=true);
      translate([-hang_w / 2 + hang_t / 2, batt_frame_d / 2 + hang_s / 2, standoff_h / 2])
        cube([hang_t, hang_s, standoff_h], center=true);

      translate([0, -batt_frame_d / 2 - hang_s, standoff_h / 2])
        cube([hang_w, hang_t, standoff_h], center=true);
      translate([hang_w / 2 - hang_t / 2, -batt_frame_d / 2 - hang_s / 2, standoff_h / 2])
        cube([hang_t, hang_s, standoff_h], center=true);
      translate([-hang_w / 2 + hang_t / 2, -batt_frame_d / 2 - hang_s / 2, standoff_h / 2])
        cube([hang_t, hang_s, standoff_h], center=true);
    }

    // translate([batt_frame_w / 2 - 3, 0, standoff_h / 2])
    //   #cube([3.0, 15, standoff_h + 0.1], center=true);
    // translate([-batt_frame_w / 2 + 3, 0, standoff_h / 2])
    //   cube([3.0, 15, standoff_h + 0.1], center=true);
  }
}

module encoder_frame(show_encoder=1, show_motor=0) {
  mount_w = 18;
  mount_d = 11;

  motor_standoff_w = 5;
  motor_standoff_h = 12;
  encoder_standoff_w = 7;
  encoder_standoff_h = 6;
  support_w = 4;
  support_h = 3.5;

  motor_mount_d = 20.0;

  if (show_motor) {
    rotate(45)
      translate([0, 0, motor_standoff_h])
        gimbal_motor();
  }

  difference() {
      union() {
      // Show encoder
      if (show_encoder) {
        translate([0, 0, encoder_standoff_h + 0.01]) encoder_board();
      }

      // Encoder frame
      frame(mount_w, mount_d,
            M2_screw_w, M2_nut_w, M2_nut_h,
            encoder_standoff_w, encoder_standoff_h,
            support_w, support_h);

      // Motor frame
      rotate(45)
        frame(motor_mount_d, motor_mount_d,
              M2_screw_w, M2_nut_w, M2_nut_h,
              motor_standoff_w, motor_standoff_h,
              support_w, support_h,
              disable=[2]);
    }

    // Mount holes
    hole_positions = [
      [mount_w / 2, mount_d / 2, encoder_standoff_h / 2],
      [mount_w / 2, -mount_d / 2, encoder_standoff_h / 2],
      [-mount_w / 2, mount_d / 2, encoder_standoff_h / 2],
      [-mount_w / 2, -mount_d / 2, encoder_standoff_h / 2]
    ];
    for (hole_pos = hole_positions) {
      translate(hole_pos) {
        cylinder(r=M2_screw_w / 2, h=encoder_standoff_h + 0.1, center=true);

        translate([0, 0, -encoder_standoff_h / 2 + M2_nut_h / 2])
          cylinder(r=M2_nut_w / 2, h=M2_nut_h + 0.1, $fn=6, center=true);
      }
    }
  }
}

module landing_frame(w, d) {
  leg_w = 5.0;
  leg_l = 40.0;
  leg_h = 5.0;

  difference() {
    union() {
      // Frame mount
      frame(w, d,
            M2_screw_w, M2_nut_w, M2_nut_h,
            standoff_w, standoff_h, standoff_w, standoff_h);

      // Feet
      N = sqrt(leg_w * leg_w + leg_w * leg_w);
      feet_h = 5.0;
      feet_positions = [
        [w / 2 + N, d / 2 + N, feet_h / 2, 45],
        [w / 2 + N, -d / 2 - N, feet_h / 2, -45],
        [-w / 2 - N, d / 2 + N, feet_h / 2, -45],
        [-w / 2 - N, -d / 2 - N, feet_h / 2, 45]
      ];
      for (feet_pos = feet_positions) {
        x = feet_pos[0];
        y = feet_pos[1];
        z = feet_pos[2];
        rotz = feet_pos[3];
        translate([x, y, z]) {
          rotate([0, 0, rotz])
          cube([leg_l, leg_w, feet_h], center=true);
        }
      }
    }

    // Counter sink holes
    positions = [
      [w / 2, d / 2, standoff_h / 4],
      [-w / 2, d / 2, standoff_h / 4],
      [w / 2, -d / 2, standoff_h / 4],
      [-w / 2, -d / 2, standoff_h / 4]
    ];
    for (pos = positions) {
      translate(pos) {
        cylinder(r=M2_screw_w / 2, h=10.0, center=true);
        cylinder(r=4/2, h=standoff_h / 2 + 0.01, center=true);
      }
    }

  }
}

module landing_feet() {
  feet_l = 30.0;
  feet_w = 10.0;
  feet_h = 8.0;
  support_h = 20.0 - 4;

  difference() {
    union() {
      // Support
      translate([(feet_l - feet_w) / 2, 0, support_h / 2])
        cylinder(r=feet_w / 2, h=support_h, center=true);

      // Feet
      translate([0, 0, feet_h / 2])
        cube([feet_l, feet_w, feet_h], center=true);
    }

    // Support hole
    translate([(feet_l - feet_w) / 2, 0, support_h / 2])
      cylinder(r=M3_screw_w / 2, h=support_h + 0.1, center=true);
    translate([(feet_l - feet_w) / 2, 0, (M3_caphead_h + 1) / 2])
      cylinder(r=M3_caphead_w / 2, h=(M3_caphead_h + 1) + 0.1, center=true);
  }
}

module odroid_frame(mount_w, mount_d, show_odroid=0) {
  // Show Odroid XU4
  if (show_odroid) {
    color([1, 0.0, 0.0])
      rotate([90.0, 0.0, 90.0])
        translate([-odroid_d / 2, 5.2, -odroid_w / 2])
          import("../proto_parts/Odroid_XU4/Odroid_XU4.STL");
  }

  h = 4.0;
  x = odroid_mount_w / 2.0;
  y = odroid_mount_d / 2.0;
  z = standoff_h + h / 2.0;
  odroid_mount_positions = [[x, y, z], [x, -y, z], [-x, y, z], [-x, -y, z]];
  mount_positions = [
    [mount_w / 2.0, mount_d / 2.0, standoff_h / 2.0],
    [mount_w / 2.0, -mount_d / 2.0, standoff_h / 2.0],
    [-mount_w / 2.0, mount_d / 2.0, standoff_h / 2.0],
    [-mount_w / 2.0, -mount_d / 2.0, standoff_h / 2.0]
  ];

  difference() {
    union() {
      // Mount frame
      frame(mount_w, mount_d,
            M3_screw_w, M3_nut_w, M3_nut_h,
            standoff_w, standoff_h, standoff_w, standoff_h, 0, 0);

      // Odroid frame
      frame(odroid_mount_w, odroid_mount_d,
            M3_screw_w, M3_nut_w, M3_nut_h,
            standoff_w, standoff_h, standoff_w, standoff_h, 0, 1);

      // Mount supports
      for (pos = odroid_mount_positions)
        translate(pos)
          cylinder(r=(M3_screw_w + 2.0) / 2, h=h, center=true);
    }

    // Mount holes
    for (pos = mount_positions) {
      translate(pos)
        cylinder(r=M3_screw_w / 2, h=20, center=true);
    }

    // Odroid mount holes
    for (pos = odroid_mount_positions) {
      translate(pos)
        cylinder(r=M3_screw_w / 2, h=20, center=true);
    }
  }
}

module stereo_camera_frame(show_cameras=1) {
  mount_w = 18.5;
  baseline = stereo_baseline;
  camera_mount_w = 24.5;

  // Mount frame
  frame(mount_w, mount_w,
        M2_screw_w, M2_nut_w, M2_nut_h,
        standoff_w - 3, standoff_h, standoff_w - 3, standoff_h);

  // // Mount frame
  // frame(0.0, 30.0,
  //       M2_screw_w, M2_nut_w, M2_nut_h,
  //       standoff_w - 3, standoff_h, standoff_w - 3, standoff_h,
  //       0, 1);

  // Board-cameras
  if (show_cameras) {
    translate([0.0, baseline / 2.0, standoff_h + 3.0]) board_camera();
    translate([0.0, -baseline / 2.0, standoff_h + 3.0]) board_camera();
  }

  // Board camera frames
  translate([0.0, baseline / 2.0, 0.0])
    frame(camera_mount_w, camera_mount_w,
          M2_screw_w, M2_nut_w, M2_nut_h,
          standoff_w - 3, standoff_h, standoff_w - 3, standoff_h,
          0, 1);
  translate([0.0, -baseline / 2.0, 0.0])
    frame(camera_mount_w, camera_mount_w,
          M2_screw_w, M2_nut_w, M2_nut_h,
          standoff_w - 3, standoff_h, standoff_w - 3, standoff_h,
          0, 1);

  // Board camera standoffs
  translate([0.0, baseline / 2.0, standoff_h + 3 / 2]) {
    mount_w = 24.5;
    x = mount_w / 2.0;
    y = mount_w / 2.0;
    z = 0.0;
    positions = [[x, y, z], [x, -y, z], [-x, y, z], [-x, -y, z]];
    for (pos = positions) {
      translate(pos) {
        difference() {
          cylinder(r=2.0, h=3.0, center=true);
          cylinder(r=M2_screw_w / 2.0, h=3.0 + 0.1, center=true);
        }
      }
    }
  }
  translate([0.0, -baseline / 2.0, standoff_h + 3 / 2]) {
    mount_w = 24.5;
    x = mount_w / 2.0;
    y = mount_w / 2.0;
    z = 0.0;
    positions = [[x, y, z], [x, -y, z], [-x, y, z], [-x, -y, z]];
    for (pos = positions) {
      translate(pos) {
        difference() {
          cylinder(r=2.0, h=3.0, center=true);
          cylinder(r=M2_screw_w / 2.0, h=3.0 + 0.1, center=true);
        }
      }
    }
  }

  // Supports
  translate([camera_mount_w / 2, 0.0, standoff_h / 2])
    cube([3.0, 40.5, standoff_h], center=true);
  translate([-camera_mount_w / 2, 0.0, standoff_h / 2])
    cube([3.0, 40.5, standoff_h], center=true);
}

module sbgc_frame(show_sbgc=1) {
  motor_mount_d = 20.0;
  mount_standoff_w = 6.0;
  mount_standoff_h = 4.0;
  mount_support_w = 3.0;
  mount_support_h = 4.0;

  sbgc_standoff_w = 7.0;
  sbgc_standoff_h = 8.0;
  sbgc_support_w = 3.0;
  sbgc_support_h = 4.0;

  difference() {
    union() {
      // Mount Frame
      rotate(45)
        frame(motor_mount_d, motor_mount_d,
              M3_screw_w, M3_nut_w, M3_nut_h,
              mount_standoff_w, mount_standoff_h,
              mount_support_w, mount_support_h);

      // Simple BGC
      if (show_sbgc) {
        color([1.0, 0.5, 0.0])
          rotate(90)
            translate([-sbgc_w / 2, -sbgc_d / 2 + 35, sbgc_standoff_h + 2])
              import("../proto_parts/SimpleBGC_Tiny/Tiny_revC_PCB.stl");
      }

      // SBGC frame
      translate([-35, 0, 0])
        rotate(90)
          frame(sbgc_mount_w, sbgc_mount_d,
                M2_screw_w, 0, 0,
                sbgc_standoff_w, sbgc_standoff_h,
                sbgc_support_w, sbgc_support_h,
                0, 1,
                [1, 3]);

      // Supports
      translate([-35 / 2 - 3, sbgc_mount_w / 2, sbgc_support_h / 2])
        cube([35.0, sbgc_support_w, sbgc_support_h], center=true);
      translate([-35 / 2 - 3, -sbgc_mount_w / 2, sbgc_support_h / 2])
        cube([35.0, sbgc_support_w, sbgc_support_h], center=true);

      // Cable frame
      translate([-35, 0, sbgc_support_h / 2]) {
        translate([sbgc_mount_d / 2, 0, 0])
          cube([3, 40, sbgc_support_h], center=true);
        translate([-sbgc_mount_d / 2, 0, 0])
          cube([3, 40, sbgc_support_h], center=true);

        translate([0, 20 - sbgc_support_w / 2, 0])
          cube([sbgc_mount_d, sbgc_support_w, sbgc_support_h], center=true);
        translate([0, -20 + sbgc_support_w / 2, 0])
          cube([sbgc_mount_d, sbgc_support_w, sbgc_support_h], center=true);
      }
    }

    // Mount holes
    translate([-35 - sbgc_mount_d / 2, sbgc_mount_w / 2, sbgc_standoff_h / 2])
      cylinder(r=M2_screw_w / 2, h=sbgc_standoff_h + 0.01, center=true);
    translate([-35 + sbgc_mount_d / 2, -sbgc_mount_w / 2, sbgc_standoff_h / 2])
      cylinder(r=M2_screw_w / 2, h=sbgc_standoff_h + 0.01, center=true);
    translate([-35 - sbgc_mount_d / 2, sbgc_mount_w / 2, M2_nut_h / 2])
      cylinder(r=M2_nut_w / 2, h=M2_nut_h + 0.01, $fn=6, center=true);
    translate([-35 + sbgc_mount_d / 2, -sbgc_mount_w / 2, M2_nut_h / 2])
      cylinder(r=M2_nut_w / 2, h=M2_nut_h + 0.01, $fn=6, center=true);
  }
}


// Gimbal Motor
module gimbal_motor(has_encoders=0) {
  motor_r = 35.0 / 2.0;
  motor_h = (has_encoders) ? 25.0 : 15.0;
  base_mount_d = (has_encoders) ? 20.0 : 29.0;

  color([0.2, 0.2, 0.2])
  difference() {
    union() {
      // Main body
      cylinder(r=motor_r, h=motor_h);

      // Bottom shaft
      translate([0, 0, -3 / 2])
        cylinder(r=14 / 2, h=3, $fn=6, center=true);
    }

    // Top mount holes
    for (i = [45:90:360]) {
      rotate([0.0, 0.0, i])
        translate([19.0 / 2.0, 0.0, motor_h - 2.0 + 0.01])
          cylinder(r=1.0, h=2.0, center=false);
    }

    // Base mount holes
    for (i = [1 : 4]) {
      rotate([0.0, 0.0, i * 90.0 + 45.0])
        translate([base_mount_d / 2.0, 0.0, -0.01])
          cylinder(r=1.0, h=2.0, center=false);
    }

    // Wire hole
    if (has_encoders) {
      translate([-motor_r + 2.5, 0.0, 2.0 - 0.01])
        cube([5.0, 9.0, 4.0], center=true);
    }
  }
}

module gimbal_pitch_frame(has_encoders=0, show_camera=1, show_motor=1, show_imu=1, dual_motors=1) {
  motor_h = (has_encoders) ? 25.0 : 15.0;
  camera_mount_w = 18.5;
  motor_mount_w = 13.2;
  pivot_h = 5.0;
  pivot_disc_h = 2.0;
  pivot_disc_r = 7.0 / 2.0;

  // Camera
  if (show_camera) {
    translate([-12.0, 0, pitch_frame_w / 2 - standoff_h / 2])
      rotate([90, 0, 90])
        stereo_camera_frame();
  }

  // Motor
  if (show_motor) {
    translate([0.0, 0.0, -motor_h + 0.01])
      gimbal_motor(has_encoders);

    if (dual_motors) {
      translate([0.0, 0.0, pitch_frame_w - standoff_h + motor_h + 0.01])
        rotate([180, 0, 0])
          gimbal_motor(has_encoders);
    }
  }

  // IMU
  if (show_imu) {
    translate([0,  20, pitch_frame_w / 2 - standoff_h / 2])
      rotate([-90, 180, -90])
        translate([0, 0, 6])
          mpu6050();
  }

  // IMU mount
  imu_mount_w = 15.5;
  rotate([0, -90, 0])
    translate([pitch_frame_w / 2 - standoff_h / 2, 25, (standoff_h + 2) / 2])
        rotate([180.0, 0, 0])
        frame(15.5, 0,
              M2_screw_w, M2_nut_w, M2_nut_h,
              standoff_w - 3, standoff_h + 5,
              standoff_w - 3, standoff_h + 2,
              0, 1);
  translate([0.0, 22.0, pitch_frame_w / 2 - standoff_h / 2])
    cube([6.0, 6.0, 10.0], center=true);

  // Motor mount
  frame(motor_mount_w, motor_mount_w,
        M2_screw_w, M2_nut_w, M2_nut_h,
        standoff_w, standoff_h, standoff_w, standoff_h,
        0, 0);
  translate([motor_mount_w / 2 + 1.2, 0, standoff_h / 2])
    cube([6.0, 10.0, standoff_h], center=true);
  translate([-motor_mount_w / 2 - 1.2, 0, standoff_h / 2])
    cube([6.0, 10.0, standoff_h], center=true);
  translate([0, motor_mount_w / 2 + 1.2, standoff_h / 2])
    cube([10.0, 6.0, standoff_h], center=true);
  translate([0, -motor_mount_w / 2 - 1.2, standoff_h / 2])
    cube([10.0, 6.0, standoff_h], center=true);

  if (dual_motors) {
    translate([0, 0, pitch_frame_w - standoff_h * 2]) {
      frame(motor_mount_w, motor_mount_w,
            M2_screw_w, M2_nut_w, M2_nut_h,
            standoff_w, standoff_h, standoff_w, standoff_h,
            0, 0);
      translate([motor_mount_w / 2 + 1.2, 0, standoff_h / 2])
        cube([6.0, 10.0, standoff_h], center=true);
      translate([-motor_mount_w / 2 - 1.2, 0, standoff_h / 2])
        cube([6.0, 10.0, standoff_h], center=true);
      translate([0, motor_mount_w / 2 + 1.2, standoff_h / 2])
        cube([10.0, 6.0, standoff_h], center=true);
      translate([0, -motor_mount_w / 2 - 1.2, standoff_h / 2])
        cube([10.0, 6.0, standoff_h], center=true);
    }
  }

  // Supports
  difference() {
    x = 0;
    y = 19.0;
    h = pitch_frame_w - standoff_h;
    union() {
      // Main support body
      translate([-x, -y, h / 2])
        cube([standoff_h + 2, standoff_h, h], center=true);
      translate([-x, y, h / 2])
        cube([standoff_h + 2, standoff_h, h], center=true);
      translate([-x, 0, standoff_h / 2])
        cube([standoff_h + 2, 36 + 2, standoff_h], center=true);
      translate([-x, 0, standoff_h / 2 + h - standoff_h])
        cube([standoff_h + 2, 36 + 2, standoff_h], center=true);

      // Camera mount supports
      translate([-x, 0, pitch_frame_w / 2 - standoff_h / 2 + camera_mount_w / 2])
        cube([standoff_h + 2, 36 + 2, standoff_h + 3], center=true);
      translate([-x, 0, pitch_frame_w / 2 - standoff_h / 2 - camera_mount_w / 2])
        cube([standoff_h + 2, 36 + 2, standoff_h + 3], center=true);
    }

    // Camera mount holes
    translate([-x, camera_mount_w / 2, pitch_frame_w / 2 - standoff_h / 2 + camera_mount_w / 2])
      rotate([0, 90, 0])
        cylinder(r=M2_screw_w / 2, h=standoff_h + 2.01, center=true);
    translate([-x, -camera_mount_w / 2, pitch_frame_w / 2 - standoff_h / 2 + camera_mount_w / 2])
      rotate([0, 90, 0])
        cylinder(r=M2_screw_w / 2, h=standoff_h + 2.01, center=true);
    translate([-x, camera_mount_w / 2, pitch_frame_w / 2 - standoff_h / 2 - camera_mount_w / 2])
      rotate([0, 90, 0])
        cylinder(r=M2_screw_w / 2, h=standoff_h + 2.01, center=true);
    translate([-x, -camera_mount_w / 2, pitch_frame_w / 2 - standoff_h / 2 - camera_mount_w / 2])
      rotate([0, 90, 0])
        cylinder(r=M2_screw_w / 2, h=standoff_h + 2.01, center=true);

    // Camera mount counter-sink holes
    z0 = pitch_frame_w / 2 - standoff_h / 2 + camera_mount_w / 2;
    z1 = pitch_frame_w / 2 - standoff_h / 2 - camera_mount_w / 2;
    translate([-x + M2_nut_h, camera_mount_w / 2, z0])
      rotate([0, 90, 0])
        cylinder(r=M2_nut_w / 2, h=M2_nut_h + 0.1, $fn=6, center=true);
    translate([-x + M2_nut_h, -camera_mount_w / 2, z0])
      rotate([0, 90, 0])
        cylinder(r=M2_nut_w / 2, h=M2_nut_h + 0.1, $fn=6, center=true);
    translate([-x + M2_nut_h, camera_mount_w / 2, z1])
      rotate([0, 90, 0])
        cylinder(r=M2_nut_w / 2, h=M2_nut_h + 0.1, $fn=6, center=true);
    translate([-x + M2_nut_h, -camera_mount_w / 2, z1])
      rotate([0, 90, 0])
        cylinder(r=M2_nut_w / 2, h=M2_nut_h + 0.1, $fn=6, center=true);


    // Trim the base end
    translate([0, 0, standoff_h / 2])
      cube([6.0 + 0.1, 10.0 + 0.3, standoff_h + 0.1], center=true);
    if (dual_motors) {
      translate([0, 0, h - standoff_h / 2])
        cube([6.0 + 0.1, 10.0 + 0.3, standoff_h + 0.1], center=true);
    }
  }

  if (dual_motors == 0) {
    // Pivot
    translate([0, 0, pitch_frame_w - standoff_h + pivot_h / 2])
      cylinder(r=5/2, h=pivot_h, center=true);
    translate([0, 0, pitch_frame_w - standoff_h + 0.5])
      cylinder(r=7/2, h=1, center=true);

    // Pivot disc
    translate([0, 0, pitch_frame_w - standoff_h - pivot_disc_h / 2])
      cylinder(r=pivot_disc_r, h=pivot_disc_h, center=true);
  }
}

module gimbal_roll_pivot_frame(has_encoders=0) {
  pivot_frame_h = 6.0;
  base_mount_d = (has_encoders) ? 14.2 : 20.5;
  mount_w = roll_rod_mount_w;
  mount_d = roll_rod_mount_d;
  mount_h = 6.0;

  difference() {
    // Body
    union() {
      // Main Body
      translate([0, 0, mount_h / 2])
        cylinder(r=20.0 / 2, h=mount_h, center=true);

      // Mount
      translate([-mount_w, mount_d, mount_h / 2])
        cylinder(r=standoff_w / 2, h=mount_h, center=true);
      translate([-mount_w, -mount_d, mount_h / 2])
        cylinder(r=standoff_w / 2, h=mount_h, center=true);

      // Join body and mount together
      translate([-mount_w / 2, 0, mount_h / 2])
        cube([mount_w, 20.0 + standoff_w, mount_h], center=true);
      translate([0, mount_d, mount_h / 2])
        cylinder(r=standoff_w / 2, h=mount_h, center=true);
      translate([0, -mount_d, mount_h / 2])
        cylinder(r=standoff_w / 2, h=mount_h, center=true);
    }

    // Mount holes
    translate([-mount_w, mount_d, mount_h / 2])
      cylinder(r=M3_screw_w / 2, h=mount_h + 0.1, center=true);
    translate([-mount_w, -mount_d, mount_h / 2])
      cylinder(r=M3_screw_w / 2, h=mount_h + 0.1, center=true);

    // Bearing counter-sink
    translate([0, 0, mount_h / 2 + 1])
      cylinder(r=14.2 / 2, h=mount_h, center=true);
    cylinder(r=5.2 / 2, h=mount_h, center=true);
  }
}

module gimbal_roll_motor_frame(has_encoders=0) {
  motor_r = 35.0 / 2.0;
  motor_h = (has_encoders) ? 25.0 : 15.0;
  motor_mount_w = 13.2;
  motor_mount_d = (has_encoders) ? 14.2 : 20.5;
  motor_mount_h = standoff_h;

  mount_w = roll_rod_mount_w;
  mount_d = roll_rod_mount_d;
  mount_h = roll_rod_mount_h;

  // Roll frame
  difference() {
    union() {
      // Motor frame
      // translate([motor_mount_d / 2, motor_mount_d / 2, motor_mount_h / 2])
      //   cylinder(r=standoff_w / 2.0, h=motor_mount_h, center=true);
      // translate([motor_mount_d / 2, -motor_mount_d / 2, motor_mount_h / 2])
      //   cylinder(r=standoff_w / 2.0, h=motor_mount_h, center=true);
      // translate([-motor_mount_d / 2, motor_mount_d / 2, motor_mount_h / 2])
      //   cylinder(r=standoff_w / 2.0, h=motor_mount_h, center=true);
      // translate([-motor_mount_d / 2, -motor_mount_d / 2, motor_mount_h / 2])
      //   cylinder(r=standoff_w / 2.0, h=motor_mount_h, center=true);
      // translate([0, motor_mount_d / 2, standoff_h / 2])
      //   cube([motor_mount_d, standoff_w, standoff_h], center=true);
      // translate([0, -motor_mount_d / 2, standoff_h / 2])
      //   cube([motor_mount_d, standoff_w, standoff_h], center=true);
      // translate([motor_mount_d / 2, 0.0, standoff_h / 2])
      //   cube([standoff_w, motor_mount_d, standoff_h], center=true);
      // translate([-motor_mount_d / 2, 0.0, standoff_h / 2])
      //   cube([standoff_w, motor_mount_d, standoff_h], center=true);
      translate([0, 0, standoff_h / 2])
        cylinder(r=19, h=standoff_h, center=true);


      // Roll bar mount
      translate([-mount_w, mount_d, mount_h / 2])
        cylinder(r=standoff_w / 2.0, h=mount_h, center=true);
      translate([-mount_w, -mount_d, mount_h / 2])
        cylinder(r=standoff_w / 2.0, h=mount_h, center=true);

      translate([motor_mount_d / 2, mount_d, standoff_h / 2])
        cylinder(r=standoff_w / 2.0, h=standoff_h, center=true);
      translate([motor_mount_d / 2, -mount_d, standoff_h / 2])
        cylinder(r=standoff_w / 2.0, h=standoff_h, center=true);

      x = (motor_mount_d / 2) + (mount_w - (motor_mount_d / 2)) / 2 - motor_mount_d / 2;
      w = (mount_w + (motor_mount_d / 2));
      translate([-x, mount_d, standoff_h / 2])
        cube([w, standoff_w, standoff_h], center=true);
      translate([-x, -mount_d, standoff_h / 2])
        cube([w, standoff_w, standoff_h], center=true);
      translate([-mount_w, 0.0, standoff_h / 2])
        cube([standoff_w, mount_d * 2, standoff_h], center=true);
      translate([motor_mount_d / 2, 0.0, standoff_h / 2])
        cube([standoff_w, mount_d * 2, standoff_h], center=true);
    }

    // Gimbal motor center hole
    translate([0, 0, standoff_h / 2])
      cylinder(r=9, h=standoff_h + 0.1, center=true);

    // Roll bar holes
    translate([-mount_w, mount_d, mount_h / 2])
      cylinder(r=M3_screw_w / 2.0, h=mount_h + 0.1, center=true);
    translate([-mount_w, -mount_d, mount_h / 2])
      cylinder(r=M3_screw_w / 2.0, h=mount_h + 0.1, center=true);
    translate([-mount_w, mount_d, M3_nut_h / 2])
      cylinder(r=M3_nut_w / 2.0, h=M3_nut_h + 0.1, $fn=6, center=true);
    translate([-mount_w, -mount_d, M3_nut_h / 2])
      cylinder(r=M3_nut_w / 2.0, h=M3_nut_h + 0.1, $fn=6, center=true);

    // Motor mount holes
    rotate(45) {
    translate([-motor_mount_d / 2, motor_mount_d / 2, motor_mount_h / 2])
      cylinder(r=M2_screw_w / 2.0, h=motor_mount_h + 0.01, center=true);
    translate([-motor_mount_d / 2, -motor_mount_d / 2, motor_mount_h / 2])
      cylinder(r=M2_screw_w / 2.0, h=motor_mount_h + 0.01, center=true);
    translate([motor_mount_d / 2, -motor_mount_d / 2, motor_mount_h / 2])
      cylinder(r=M2_screw_w / 2.0, h=motor_mount_h + 0.01, center=true);
    translate([motor_mount_d / 2, motor_mount_d / 2, motor_mount_h / 2])
      cylinder(r=M2_screw_w / 2.0, h=motor_mount_h + 0.01, center=true);
    }
  }
}

module gimbal_roll_bar_frame(has_encoders=0) {
  motor_mount_w = 13.2;
  motor_mount_d = (has_encoders) ? 14.2 : 20.5;
  motor_mount_h = 8.0;

  roll_bar_w = 45;
  min_y = -1 * roll_bar_w / 2.0;
  max_y = roll_bar_w / 2.0;

  support_h = 6;

  difference() {
    union() {
      // Motor frame
      translate([motor_mount_w / 2, motor_mount_w / 2, motor_mount_h / 2])
        cylinder(r=standoff_w / 2.0, h=motor_mount_h, center=true);
      translate([motor_mount_w / 2, -motor_mount_w / 2, motor_mount_h / 2])
        cylinder(r=standoff_w / 2.0, h=motor_mount_h, center=true);
      translate([-motor_mount_w / 2, motor_mount_w / 2, motor_mount_h / 2])
        cylinder(r=standoff_w / 2.0, h=motor_mount_h, center=true);
      translate([-motor_mount_w / 2, -motor_mount_w / 2, motor_mount_h / 2])
        cylinder(r=standoff_w / 2.0, h=motor_mount_h, center=true);
      translate([0, motor_mount_w / 2, motor_mount_h / 2])
        cube([motor_mount_w, standoff_w, motor_mount_h], center=true);
      translate([0, -motor_mount_w / 2, motor_mount_h / 2])
        cube([motor_mount_w, standoff_w, motor_mount_h], center=true);
      translate([motor_mount_w / 2, 0.0, motor_mount_h / 2])
        cube([standoff_w, motor_mount_w, motor_mount_h], center=true);
      translate([-motor_mount_w / 2, 0.0, motor_mount_h / 2])
        cube([standoff_w, motor_mount_w, motor_mount_h], center=true);

      // Rod mount body
      translate([motor_mount_d / 2, 0, (standoff_w - 1) / 2])
        rotate([90, 0, 0])
          cube([standoff_w, standoff_w -1, roll_bar_w], center=true);
      translate([-motor_mount_d / 2, 0, (standoff_w - 1) / 2])
        rotate([90, 0, 0])
          cube([standoff_w, standoff_w -1, roll_bar_w], center=true);
    }

    // Motor mount holes
    translate([-motor_mount_w / 2, motor_mount_w / 2, motor_mount_h / 2])
      cylinder(r=M2_screw_w / 2.0, h=motor_mount_h + 0.01, center=true);
    translate([-motor_mount_w / 2, -motor_mount_w / 2, motor_mount_h / 2])
      cylinder(r=M2_screw_w / 2.0, h=motor_mount_h + 0.01, center=true);
    translate([motor_mount_w / 2, -motor_mount_w / 2, motor_mount_h / 2])
      cylinder(r=M2_screw_w / 2.0, h=motor_mount_h + 0.01, center=true);
    translate([motor_mount_w / 2, motor_mount_w / 2, motor_mount_h / 2])
      cylinder(r=M2_screw_w / 2.0, h=motor_mount_h + 0.01, center=true);

    // Rod mount holes
    translate([motor_mount_d / 2, 0, (standoff_w - 1) / 2])
      rotate([90, 0, 0])
        cylinder(r=M3_screw_w / 2, h=roll_bar_w + 30 + 0.1, center=true);
    translate([-motor_mount_d / 2, 0, (standoff_w - 1) / 2])
      rotate([90, 0, 0])
        cylinder(r=M3_screw_w / 2, h=roll_bar_w + 30 + 0.1, center=true);

    // Center hole
    translate([0, 0, motor_mount_h / 2])
      cylinder(r=10 / 2, h=motor_mount_h + 0.1, center=true);
  }
}

module gimbal_roll_frame(has_encoders=0, show_pitch_frame=1, dual_motors=1) {
  motor_r = 35.0 / 2.0;
  motor_h = (has_encoders) ? 25.0 : 15.0;
  motor_mount_w = 13.2;
  motor_mount_d = (has_encoders) ? 14.2 : 20.5;
  motor_mount_h = 8.0;

  mount_w = roll_rod_mount_w;
  mount_d = roll_rod_mount_d;
  mount_h = roll_rod_mount_h;

  // Pitch frame
  if (show_pitch_frame) {
    // Pitch frame
    translate([0, pitch_frame_w / 2 - standoff_h / 2, mount_w + 4])
      rotate([90, -90, 0])
        gimbal_pitch_frame(has_encoders, dual_motors=dual_motors);

    // Roll rods
    color([0.4, 0.4, 0.4]) {
      translate([mount_d, 0, 4])
        rotate([90, 0, 0])
          cylinder(r=3 / 2, h=150, center=true);
      translate([-mount_d, 0, 4])
        rotate([90, 0, 0])
          cylinder(r=3 / 2, h=150, center=true);
    }
  }

  // Roll motor frame
  translate([0, pitch_frame_w / 2 + standoff_h / 2 + motor_h, 36])
    rotate([90, -90, 0])
      gimbal_roll_motor_frame();
  if (dual_motors) {
    translate([0, -pitch_frame_w / 2 - standoff_h / 2 - motor_h, 36])
      rotate([-90, -90, 0])
        gimbal_roll_motor_frame();

    translate([0, -pitch_frame_w / 2 - standoff_h / 2 - motor_h - 8, 36])
      rotate([-90, -180, 0])
        encoder_frame();

  }

  // Roll mount frame
  if (show_pitch_frame) {
    gimbal_roll_bar_frame();

    translate([0, 0, -motor_h])
      gimbal_motor(has_encoders);
  }
}

module gimbal_yaw_frame(show_roll_frame=1, show_sbgc_frame=1) {
  has_encoders = 0;
  motor_h = (has_encoders) ? 25.0 : 15.0;
  motor_mount_w = 13.2;
  motor_mount_d = (has_encoders) ? 14.2 : 20.5;

  standoff_w = 8.0;
  standoff_h = 8.0;

  support_w = 8.0;
  support_h = 8.0;

  encoder_h = 7.5;
  roll_mount_h = 50;
  roll_mount_d = 48;

  // Show roll frame
  if (show_roll_frame) {
    rotate([0, 90, 0])
      translate([-roll_mount_h, 0.0, -roll_mount_d + motor_h + encoder_h - 0.01])
        gimbal_roll_frame();

    rotate([0, 90, 0])
      translate([-roll_mount_h, 0.0, -roll_mount_d])
        rotate([0, 0, 90])
          encoder_frame(1);
  }

  // Show SBGC frame
  if (show_sbgc_frame) {
    rotate([0, -90, 0]) {
      translate([roll_mount_h, 0.0, roll_mount_d + standoff_h]) {
        sbgc_frame();
      }
    }
  }

  // Yaw frame
  difference() {
    union() {
      // Yaw motor mount
      frame(motor_mount_w, motor_mount_w,
            M2_screw_w, M2_nut_w, M2_nut_h,
            standoff_w, standoff_h,
            support_w, support_h);

      // Roll motor mount
      // rotate([0, 90, 0])
      //   translate([-roll_mount_h, 0.0, -roll_mount_d - standoff_h])
            // frame(motor_mount_d, motor_mount_d,
            //       M2_screw_w, M2_nut_w, M2_nut_h,
            //       standoff_w, standoff_h,
            //       support_w, support_h);

      // Horizontal supports
      hx = motor_mount_d / 2 + roll_mount_d + support_h;
      hy = motor_mount_w / 2 + standoff_w;
      translate([motor_mount_d / 2, 0, support_h / 2]) {
        translate([-hx / 2, hy, 0])
          cube([hx, support_w, support_h], center=true);
        translate([-hx / 2, -hy, 0])
          cube([hx, support_w, support_h], center=true);
        translate([-hx + support_w / 2, 0, 0])
          cube([support_w, motor_mount_d + support_w, support_h], center=true);
        translate([-hx + support_w / 2, 0, roll_mount_h - motor_mount_d / 2 - support_h])
          cube([support_w, hy * 2 + support_w, support_h], center=true);
        translate([-hx + support_w / 2, 0, roll_mount_h + motor_mount_d / 2])
          cube([support_w, hy * 2 + support_w, support_h], center=true);
      }
      translate([motor_mount_d / 2, 0, support_h / 2])
        cube([support_w, hy * 2 + support_w, support_h], center=true);
      translate([-motor_mount_d / 2, 0, support_h / 2])
        cube([support_w, hy * 2 + support_w, support_h], center=true);

      // Vertical supports
      vx = roll_mount_d + standoff_h / 2;
      vy = motor_mount_w / 2 + standoff_w;
      vz = roll_mount_h + motor_mount_d / 2 + support_w / 2;
      translate([-vx, vy, vz / 2])
        cube([support_h, support_w, vz], center=true);
      translate([-vx, -vy, vz / 2])
        cube([support_h, support_w, vz], center=true);

      // Diagonal supports
      dl = 50;
      dz = 16;
      dx_offset = 20;
      dx = roll_mount_d + standoff_h / 2 - dx_offset;
      dy = motor_mount_w / 2 + standoff_w;
      translate([-dx, dy, dz])
        rotate([0, -60, 0])
          cube([support_h, support_w, dl], center=true);
      translate([-dx, -dy, dz])
        rotate([0, -60, 0])
          cube([support_h, support_w, dl], center=true);
    }

    // Yaw motor mount holes
    for (i = [1:4]) {
      rotate(90 * i)
        translate([motor_mount_w / 2, motor_mount_w / 2, standoff_h / 2])
          cylinder(r=M2_screw_w / 2, h=support_h + 0.01, center=true);
    }
    translate([0, 0, support_h / 2])
      cylinder(r=10.0 / 2.0, h=support_h + 0.01, center=true);

    // Roll motor mount holes
    rotate([0, 90, 0]) {
      translate([-roll_mount_h, 0.0, -roll_mount_d - standoff_h]) {
        for (i = [1:4]) {
          rotate(90 * i + 45)
            translate([motor_mount_d / 2, motor_mount_d / 2, standoff_h / 2])
              #cylinder(r=M2_screw_w / 2, h=standoff_h + 0.01, center=true);
        }

        translate([0, 0, standoff_h / 2])
          cylinder(r=20.0 / 2.0, h=standoff_h + 0.01, center=true);
      }
    }
  }
}

module gimbal_frame(mount_w, mount_d, show_yaw_frame=1, show_sbgc=1, show_pololu=1) {
  has_encoders = 0;
  motor_h = (has_encoders) ? 25.0 : 15.0;
  motor_mount_d = (has_encoders) ? 14.2 : 20.5;

  roll_mount_w = 30.0;
  roll_mount_h = 50.0;
  encoder_standoff_h = 12;

  sbgc_standoff_w = 5.0;
  sbgc_standoff_base_w = 8.0;
  sbgc_standoff_h = 8.0;

  psu_standoff_w = 5.0;
  psu_standoff_base_w = 8.0;
  psu_standoff_h = 8.0;
  psu_support_w = 3.0;
  psu_support_h = 4.0;

  // Show yaw frame
  if (show_yaw_frame) {
    translate([0, 0, support_h + 0.01])
      encoder_frame();

    translate([0, 0, support_h + encoder_standoff_h +  0.01])
      rotate(45)
        gimbal_motor();

    translate([0, 0, support_h + encoder_standoff_h + motor_h])
      gimbal_yaw_frame();
  }

  // Simple BGC
  if (show_sbgc) {
    color([1.0, 0.5, 0.0])
      translate([-sbgc_w / 2, -sbgc_d / 2 + 35, sbgc_standoff_h + 1])
        import("../proto_parts/SimpleBGC_Tiny/Tiny_revC_PCB.stl");
  }

  // Pololu PSU
  if (show_pololu) {
    rotate(90)
    color([0.0, 1.0, 0.0])
      translate([-pololu_w / 2, -pololu_d / 2 - 32, psu_standoff_h])
        import("../proto_parts/Pololu_U3V50X/Pololu-U3V50X.STL");
  }

  difference() {
    union() {
      // Yaw motor mount
      translate([0, 0, support_h / 2])
        cylinder(r=20, h=support_h, center=true);

      // Mount frame
      frame(mount_w, mount_d,
            M3_screw_w, M3_nut_w, M3_nut_h,
            standoff_w, support_h,
            support_w, support_h);

      // SBGC frame
      sbgc_holes = [
        [sbgc_mount_w / 2,  35.0 + sbgc_mount_d / 2, 0],
        [-sbgc_mount_w / 2, 35.0 + -sbgc_mount_d / 2, 0]
      ];
      for (pos = sbgc_holes) {
        translate(pos) {
          translate([0, 0, sbgc_standoff_h / 2])
            cylinder(r=sbgc_standoff_w / 2, h=sbgc_standoff_h, center=true);
          translate([0, 0, support_h / 2])
            cylinder(r=sbgc_standoff_base_w / 2, h=support_h, center=true);
        }
      }

      // Pololu PSU frame
      psu_holes = [
        [pololu_mount_w / 2,  -32.0 + pololu_mount_d / 2, 0],
        [-pololu_mount_w / 2, -32.0 + -pololu_mount_d / 2, 0]
      ];
      for (pos = psu_holes) {
        rotate(90)
        translate(pos) {
          translate([0, 0, psu_standoff_h / 2])
            cylinder(r=psu_standoff_w / 2, h=psu_standoff_h, center=true);
          translate([0, 0, support_h / 2])
            cylinder(r=psu_standoff_base_w / 2, h=support_h, center=true);
        }
      }

      // Supports
      translate([motor_mount_d / 2 + 1.5, 0, support_h / 2])
        cube([support_w, mount_d, support_h], center=true);
      translate([-motor_mount_d / 2 - 1.2, 0, support_h / 2])
        cube([support_w, mount_d, support_h], center=true);
      translate([0, motor_mount_d / 2, support_h / 2])
        cube([mount_w, support_w, support_h], center=true);
      translate([0, -motor_mount_d / 2, support_h / 2])
        cube([mount_w, support_w, support_h], center=true);

      // Pololu supports
      rotate(90) {
        translate([pololu_mount_w / 2 - 5, -32 + pololu_mount_d / 2, support_h / 2])
          cube([10, psu_standoff_base_w, support_h], center=true);
        translate([-pololu_mount_w / 2 + 5, -32 - pololu_mount_d / 2, support_h / 2])
          cube([10, psu_standoff_base_w, support_h], center=true);
      }
    }

    // Yaw motor center holes
    translate([0, 0, support_h / 2])
      cylinder(r=10, h=support_h + 0.01, center=true);

    // Yaw motor mount holes
    positions = [
      [motor_mount_d / 2, motor_mount_d / 2, support_h / 2],
      [-motor_mount_d / 2, motor_mount_d / 2, support_h / 2],
      [motor_mount_d / 2, -motor_mount_d / 2, support_h / 2],
      [-motor_mount_d / 2, -motor_mount_d / 2, support_h / 2]
    ];
    for (pos = positions) {
      rotate(45)
        translate(pos)
          cylinder(r=M2_screw_w / 2, h=support_h + 0.01, center=true);
    }

    // SBGC mount holes
    sbgc_holes = [
      [sbgc_mount_w / 2,  35.0 + sbgc_mount_d / 2, 0],
      [-sbgc_mount_w / 2, 35.0 + -sbgc_mount_d / 2, 0]
    ];
    for (pos = sbgc_holes) {
      translate(pos) {
        translate([0, 0, sbgc_standoff_h / 2])
          cylinder(r=M2_screw_w / 2, h=sbgc_standoff_h + 0.01, center=true);
        translate([0, 0, support_h / 2])
          cylinder(r=M2_nut_w / 2, h=support_h + 0.01, $fn=6, center=true);
      }
    }

    // PSU mount holes
    psu_holes = [
      [pololu_mount_w / 2,  -32.0 + pololu_mount_d / 2, 0],
      [-pololu_mount_w / 2, -32.0 + -pololu_mount_d / 2, 0]
    ];
    for (pos = psu_holes) {
      rotate(90) {
        translate(pos) {
          translate([0, 0, psu_standoff_h / 2])
            cylinder(r=M2_screw_w / 2, h=psu_standoff_h + 0.01, center=true);
          translate([0, 0, support_h / 2])
            cylinder(r=M2_nut_w / 2, h=support_h + 0.01, $fn=6, center=true);
        }
      }
    }
  }
}

module nuc_frame(mount_w, mount_d, show_nuc=0) {
  nuc_standoff_w = 6;
  nuc_standoff_h = 16;
  nuc_support_w = 6;
  nuc_support_h = 6;

  // Show NUC
  if (show_nuc) {
    color([0.0, 0.0, 1.0])
      translate([0, 0, -28 + nuc_standoff_h])
        rotate([90.0, 0.0, 0.0])
          import("../proto_parts/Intel_NUC7i5DN/NUC7i5DN.STL");
  }

  // NUC frame
  frame(mount_w, mount_d,
        M3_screw_w, M3_nut_w, M3_nut_h,
        nuc_standoff_w, nuc_standoff_h,
        nuc_support_w, nuc_support_h);

  // Supports
  translate([0, 0, nuc_support_h / 2])
    cube([nuc_support_w, mount_d, nuc_support_h], center=true);
  // translate([mount_w / 2 * 0.5, 0, nuc_support_h / 2])
  //   cube([nuc_support_w, mount_d, nuc_support_h], center=true);
  // translate([-mount_w / 2 * 0.5, 0, nuc_support_h / 2])
  //   cube([nuc_support_w, mount_d, nuc_support_h], center=true);

  translate([0, 0, nuc_support_h / 2])
    cube([mount_w, nuc_support_w, nuc_support_h], center=true);
  // translate([0, mount_d / 2 * 0.5, nuc_support_h / 2])
  //   cube([mount_w, nuc_support_w, nuc_support_h], center=true);
  // translate([0, -mount_d / 2 * 0.5, nuc_support_h / 2])
  //   cube([mount_w, nuc_support_w, nuc_support_h], center=true);

  // Cable rails
  rail_offset = 10.0;
  difference() {
    translate([0, (mount_d / 2) + 4, nuc_support_h / 2])
      cube([mount_w - 5, rail_offset, nuc_support_h], center=true);
    translate([0, (mount_d / 2) + 4, nuc_support_h / 2])
      cube([mount_w - 10, 5, nuc_support_h + 0.01], center=true);
  }
  difference() {
    translate([0, -(mount_d / 2) - 4, support_h / 2])
      cube([mount_w - 5, rail_offset, support_h], center=true);
    translate([0, -(mount_d / 2) - 4, support_h / 2])
      cube([mount_w - 10, 5, support_h + 0.01], center=true);
  }
}

module payload_frame(mount_w, mount_d,
                     standoff_w, standoff_h,
                     support_w, support_h,
                     nuc_mount_w, nuc_mount_d) {
  difference() {
    union() {
      // Payload frame
      rotate(90)
        frame(mount_w, mount_d,
              M3_screw_w, M3_nut_w, M3_nut_h,
              support_w, support_h,
              support_w, support_h);

      // NUC frame
      frame(nuc_mount_w, nuc_mount_d,
            M3_screw_w, M3_nut_w, M3_nut_h,
            standoff_w, standoff_h,
            support_w, support_h);
    }

    // Nut countersink
    positions = [
      [mount_w / 2, mount_d / 2, support_h - M3_nut_h / 2],
      [-mount_w / 2, mount_d / 2, support_h - M3_nut_h / 2],
      [mount_w / 2, -mount_d / 2, support_h - M3_nut_h / 2],
      [-mount_w / 2, -mount_d / 2, support_h - M3_nut_h / 2]
    ];
    for (pos = positions) {
      rotate(90)
        translate(pos)
          cylinder(r=M3_nut_w / 2, h=M3_nut_h + 0.01, $fn=6, center=true);
    }
  }
}

module top_stack(show_fcu=1, show_battery=1) {
  // FCU Frame
  fcu_frame(show_fcu);

  // Battery Frame
  translate([0.0, 0.0, fcu_standoff_h + fcu_h + 0.5])
    battery_frame(batt_frame_w, batt_frame_d, show_battery);
}

module perception_module() {
  rotate([0, 0, 0])
    payload_frame(mav_payload_mount_w, mav_payload_mount_d,
                  mav_frame_standoff_w, mav_payload_standoff_h,
                  mav_frame_support_w, mav_frame_support_h,
                  nuc_mount_w, nuc_mount_d);

  translate([0.0, 0.0, mav_payload_standoff_h])
      nuc_frame(nuc_mount_w, nuc_mount_d, show_nuc=1);

  // translate([0.0, 0.0, mav_payload_standoff_h + 32.0])
  //   rotate([0, 0, 180])
  //     sbgc_frame(nuc_mount_w, nuc_mount_d);

  translate([0, 0, mav_payload_standoff_h + 50.0])
      gimbal_frame(nuc_mount_w, nuc_mount_d);
}

module print() {
  // nut_tool();

  // // Top stack
  // // -- FCU frame
  // translate([0, 120, 0])
  //   fcu_frame();
  // // -- Battery frame
  // translate([80.0, 120.0, 0.0])
  //   battery_frame();
  // // -- Spacers
  // translate([120.0, 120 + 10.0, 0.0])
  //   stack_spacer(fcu_h, nut_counter_sink=1);
  // translate([120.0, 120 + 0.0, 0.0])
  //   stack_spacer(fcu_h, nut_counter_sink=1);
  // translate([120.0, 120 + -10.0, 0.0])
  //   stack_spacer(fcu_h, nut_counter_sink=1);
  // translate([120.0, 120 + -20.0, 0.0])
  //   stack_spacer(fcu_h, nut_counter_sink=1);

  // Bottom stack
  // -- Odroid Frame
  rotate(90)
    odroid_frame(nuc_mount_w, nuc_mount_d, 0);
  // -- Landing frame
  translate([170, 0, 0])
    landing_frame(odroid_mount_w, odroid_mount_d);
  // -- Odroid Spacers
  translate([220.0, 0.0, 0.0])
    stack_spacer(fcu_h, nut_counter_sink=1);
  translate([220.0, 10.0, 0.0])
    stack_spacer(fcu_h, nut_counter_sink=1);
  translate([220.0, -10.0, 0.0])
    stack_spacer(fcu_h, nut_counter_sink=1);
  translate([220.0, -20.0, 0.0])
    stack_spacer(fcu_h, nut_counter_sink=1);

  // -- Roll Frame
  translate([0.0, -100.0, 0.0])
    rotate(90) gimbal_roll_motor_frame();
  translate([25.0, -100 - 0.0, 0.0])
    stack_spacer(10.0, nut_counter_sink=1);
  translate([25.0, -100 - 10.0, 0.0])
    stack_spacer(10.0, nut_counter_sink=1);
  translate([60.0, -100.0, 0.0])
    rotate(90) gimbal_roll_pivot_frame();
  translate([90.0, -100.0 - 0.0, 0.0])
    stack_spacer(10.0, nut_counter_sink=1);
  translate([90.0, -100.0 - 10.0, 0.0])
    stack_spacer(10.0, nut_counter_sink=1);
  translate([120.0, -110.0, 0.0])
    gimbal_roll_bar_frame();
  translate([150.0, -100.0 - 0.0, 0.0])
    stack_spacer(10.0, nut_counter_sink=1);
  translate([150.0, -100.0 - 10.0, 0.0])
    stack_spacer(10.0, nut_counter_sink=1);
  translate([150.0, -100.0 - 20.0, 0.0])
    stack_spacer(10.0, nut_counter_sink=1);
  translate([150.0, -100.0 - 30.0, 0.0])
    stack_spacer(10.0, nut_counter_sink=1);

  // -- Pitch Frame
  translate([190.0, -100.0, 0.0])
    gimbal_pitch_frame(0, 0, 0);
}

// Main
// print();

// Assembly Development
// top_stack();
// perception_module();

// Component Development
// battery_frame(batt_frame_w, batt_frame_d);
// encoder_frame(0);
// fcu_frame(show_fcu);
// stack_spacer(batt_h + 2, nut_counter_sink=1);
// odroid_frame(nuc_mount_w, nuc_mount_d, 0);
// landing_frame(nuc_mount_w, nuc_mount_d);
// landing_feet();
// stereo_camera_frame();
// sbgc_frame(show_sbgc=0);
// gimbal_motor();
// gimbal_pitch_frame(0, 0, 0, 0);
// gimbal_roll_pivot_frame();
// gimbal_roll_motor_frame();
// gimbal_roll_bar_frame();
// gimbal_roll_frame();
// gimbal_yaw_frame(0);
// gimbal_frame(nuc_mount_w, nuc_mount_d, 0, 0, 0);
// nuc_frame(nuc_mount_w, nuc_mount_d, show_nuc=1);
// payload_frame(mav_payload_mount_w, mav_payload_mount_d,
//               mav_frame_support_w, mav_frame_support_h,
//               mav_frame_support_w, mav_frame_support_h,
//               nuc_mount_w, nuc_mount_d);
