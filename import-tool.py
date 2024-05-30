import bpy
from bpy_extras.io_utils import ImportHelper

bl_info = {
    "name": "Camera Calibration Import Tool",
    "author": "Ian Sloat",
    "version": (1, 0, 0),
    "blender": (2, 93, 0),
    "location": "Movie Clip Editor > Sidebar > Track > Camera > Import Profile",
    "description": "Import tool for Blender Camera Calibration Tool profiles",
    "doc_url": "https://github.com/YaannSloot/Blender-Camera-Calibration-Tool",
    "category": "Motion Tracking",
}


class ImportProfileOperator(bpy.types.Operator, ImportHelper):
    """Import calibration profile"""
    bl_idname = "import_tool.import_profile_op"
    bl_label = "Import Profile"

    filename_ext = ".txt"

    filter_glob: bpy.props.StringProperty(
        default="*.txt",
        options={'HIDDEN'},
        maxlen=255,
    )

    def execute(self, context):
        file = self.filepath
        current_clip = context.edit_movieclip
        tracking_camera = current_clip.tracking.camera
        try:
            with open(file, 'r') as f:
                args = [line.strip().split('=') for line in f.readlines()]
            args = {pair[0]: pair[1] for pair in args if len(pair) == 2}
            cam_name = args['cam_name']
            sensor_width = float(args['sensor_width'])
            focal_length = float(args['focal_length'])
            k1 = float(args['dist_k1'])
            k2 = float(args['dist_k2'])
            k3 = float(args['dist_k3'])
            sol_cov = float(args['sol_cov'])
            avg_err = float(args['avg_err'])
            tracking_camera.sensor_width = sensor_width
            tracking_camera.focal_length = focal_length
            tracking_camera.k1 = k1
            tracking_camera.k2 = k2
            tracking_camera.k3 = k3
            tracking_camera.distortion_model = 'POLYNOMIAL'
            bpy.ops.clip.camera_preset_add(name=cam_name)
            self.report({'INFO'}, f'Imported profile "{cam_name}", '
                                  f'(solution coverage(%) = {sol_cov}, avg. error(px) = {avg_err})')
        except FileNotFoundError:
            self.report({'ERROR'}, f'Could not open file "{file}"')
        except (KeyError, ValueError):
            self.report({'ERROR'}, f'Error parsing file "{file}"')
        return {"FINISHED"}


class ImportProfilePanel(bpy.types.Panel):
    bl_label = 'Import Profile'
    bl_idname = 'IMPORT_PROFILE_PT_panel'
    bl_space_type = 'CLIP_EDITOR'
    bl_region_type = 'UI'
    bl_category = 'Track'
    bl_parent_id = "CLIP_PT_tracking_camera"

    def draw(self, context):
        layout = self.layout
        row = layout.row()
        row.operator('import_tool.import_profile_op')


def register():
    bpy.utils.register_class(ImportProfileOperator)
    bpy.utils.register_class(ImportProfilePanel)


def unregister():
    bpy.utils.unregister_class(ImportProfilePanel)
    bpy.utils.unregister_class(ImportProfileOperator)


if __name__ == "__main__":
    register()
