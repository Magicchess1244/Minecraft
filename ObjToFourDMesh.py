import trimesh
import numpy as np

Dir = input("Where is the .obj file you want to make into 4D?\n")
Name = input("\nHow do you want to call the resulting file?\n")

file = open(Name, 'w')
mesh = trimesh.load(Dir)

# Manually deduplicate vertices
unique_verts = []
vertex_map = {}
new_faces = []

for i, v in enumerate(mesh.vertices):
    v_tuple = tuple(v)
    if v_tuple not in vertex_map:
        vertex_map[v_tuple] = len(unique_verts)
        unique_verts.append(v)

# Remap faces to use deduplicated vertex indices
for face in mesh.faces:
    new_face = [vertex_map[tuple(mesh.vertices[idx])] for idx in face]
    new_faces.append(new_face)

unique_verts = np.array(unique_verts)
new_faces = np.array(new_faces)

# Create two copies with deduplicated vertices
vertices_4d_w1 = np.hstack([unique_verts, -np.ones((len(unique_verts), 1))])
vertices_4d_w2 = np.hstack([unique_verts, np.ones((len(unique_verts), 1))])

# Combine both sets of vertices
all_vertices = np.vstack([vertices_4d_w1, vertices_4d_w2])

# Format vertices with mini brackets for each vertex
print("// All 4D vertices (w=-0.5and w=0.5):")
file.write("// All 4D vertices (w=-0.5and w=0.5):")
vertices_list = []
for v in all_vertices:
    vert_str = ', '.join(map(str, v))
    vertices_list.append(f"{{{vert_str}}}")
print(f"{{{', '.join(vertices_list)}}},")
file.write(f"{{{', '.join(vertices_list)}}},")

# Use deduplicated faces for both copies
faces_w1 = new_faces
faces_w2 = new_faces + len(unique_verts)

# Combine all faces
all_faces = np.vstack([faces_w1, faces_w2])

# Flatten faces to create triangle indices
triangle_indices = all_faces.flatten().tolist()

# Add connections between w=-1 and w=1 layers
n_verts = len(unique_verts)
for face in new_faces:
    edges = [(face[0], face[1]), (face[1], face[2]), (face[2], face[0])]
    for v1, v2 in edges:
        triangle_indices.extend([v1, v2, n_verts + v1])
        triangle_indices.extend([v2, n_verts + v1, n_verts + v2])

indices_str = ', '.join(map(str, triangle_indices))
print("\n// Triangle indices:")
print(f"{{{indices_str}}}")
file.write("\n// Triangle indices:")
file.write(f"{{{indices_str}}}")


print(f"\nTotal vertices: {len(all_vertices)}")
print(f"Total indices: {len(triangle_indices)}")


